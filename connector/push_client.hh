#pragma once

#include <logger.hh>
#include <util/zmq_utils.hh>
#include <util/flex_alloc.hh>
#include <util/exception.hh>
#include "client_base.hh"
#include "endpoint_client.hh"
#include "service_type_map.hh"

namespace virtdb { namespace connector {
  
  template <typename PUSH_ITEM>
  class push_client : public client_base
  {
  public:
    typedef PUSH_ITEM push_item;
    
    static const interface::pb::ConnectionType connection_type =
      interface::pb::ConnectionType::PUSH_PULL;
    
    static const interface::pb::ServiceType service_type =
      service_type_map<PUSH_ITEM, connection_type>::value;
    
  private:
    typedef std::lock_guard<std::mutex> lock;
    
    endpoint_client                   * ep_clnt_;
    zmq::context_t                      zmqctx_;
    util::zmq_socket_wrapper            socket_;
    std::mutex                          mtx_;
    
  public:
    push_client(endpoint_client & ep_clnt,
                const std::string & server)
    : client_base(ep_clnt, server),
      ep_clnt_(&ep_clnt),
      zmqctx_(1),
      socket_(zmqctx_, ZMQ_PUSH)
    {
      // this machinery makes sure we reconnect whenever the endpoint changes
      ep_clnt.watch(service_type, [this](const interface::pb::EndpointData & ep) {
        
        bool no_change = true;
        
        if( ep.name() == this->server() )
        {
          for( auto const & conn: ep.connections() )
          {
            if( conn.type() == connection_type )
            {
              if( !socket_.connected_to(conn.address().begin(),
                                        conn.address().end()) )
              {
                for( auto const & addr: conn.address() )
                {
                  try
                  {
                    lock l(mtx_);
                    socket_.reconnect(addr.c_str());
                    no_change = false;
                    break;
                  }
                  catch( const std::exception & e )
                  {
                    std::string text{e.what()};
                    LOG_ERROR("exception in connect_socket lambda function" <<
                              V_(text) <<
                              V_(addr));
                  }
                  catch( ... )
                  {
                    LOG_ERROR("unknown exception in connect_socket lambda function");
                  }
                }
              }
            }
          }
        }
        return no_change;
      });
    }
    
    virtual bool send_request(const push_item & item)
    {
      if( !socket_.valid() ) return false;
      
      zmq::message_t msg;
      int item_size = item.ByteSize();
      
      if( item_size )
      {
        try
        {
          util::flex_alloc<unsigned char, 1024> buffer(item_size);
          
          if( !item.SerializeToArray(buffer.get(), item_size) )
          {
            THROW_("couldn't serialize request data");
          }
          
          if( !socket_.get().send( buffer.get(), item_size ) )
            return false;
          else
            return true;
        }
        catch (const zmq::error_t & e)
        {
          std::string text{e.what()};
          LOG_ERROR("zeromq exception" << V_(text));
        }
        catch( const std::exception & e )
        {
          std::string text{e.what()};
          LOG_ERROR("exception caught" << V_(text));
        }
        catch( ... )
        {
          LOG_ERROR("unknown exception caught");
        }
      }
      return false;
    }
    
    virtual bool wait_valid(unsigned long ms)
    {
      return socket_.wait_valid(ms);
    }
    
    virtual void wait_valid()
    {
      socket_.wait_valid();
    }
    
    virtual ~push_client()
    {
      ep_clnt_->remove_watches(service_type);
    }
    
  private:
    push_client() = delete;
    push_client(const push_client & other)  = delete;
    push_client & operator=(const push_client &) = delete;

  };
}}
