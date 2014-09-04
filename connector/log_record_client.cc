#include "log_record_client.hh"
#include <logger.hh>
#include <util/flex_alloc.hh>
#include <util/exception.hh>

using namespace virtdb::interface;
using namespace virtdb::logger;
using namespace virtdb::util;

namespace virtdb { namespace connector {
  
  bool log_record_client::logger_ready() const
  {
    lock l(sockets_mtx_);
    return logger_push_socket_->valid();
  }
  
  bool log_record_client::get_logs_ready() const
  {
    lock l(sockets_mtx_);
    return logger_req_socket_.valid();
  }
  
  bool log_record_client::subscription_ready() const
  {
    lock l(sockets_mtx_);
    return logger_sub_socket_.valid();
  }

  log_record_client::log_record_client(endpoint_client & ep_client)
  : zmqctx_(1),
    logger_push_socket_(new util::zmq_socket_wrapper(zmqctx_,ZMQ_PUSH)),
    logger_sub_socket_(zmqctx_, ZMQ_SUB),
    logger_req_socket_(zmqctx_, ZMQ_REQ),
    log_sink_sptr_(new log_sink(logger_push_socket_)),
    worker_(std::bind(&log_record_client::worker_function,this))
  {
    process_info::set_app_name(ep_client.name());
    
    ep_client.watch(pb::ServiceType::LOG_RECORD,
                    [this](const pb::EndpointData & ep) {
                      return this->on_endpoint_data(ep);
                    });
    ep_client.watch(pb::ServiceType::GET_LOGS,
                    [this](const pb::EndpointData & ep) {
                      return this->on_endpoint_data(ep);
                    });
    worker_.start();
  }
  
  
  bool
  log_record_client::worker_function()
  {
    bool no_socket = false;
    {
      lock l(sockets_mtx_);
      if( !logger_sub_socket_.valid() ) no_socket = true;
    }
    
    if( no_socket )
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      return true;
    }
    else
    {
      try
      {
        if( !logger_sub_socket_.poll_in(3000) )
          return true;
      }
      catch (const zmq::error_t & e)
      {
        std::string text{e.what()};
        LOG_ERROR("zmq::poll failed with exception" << V_(text) << "delaying subscription loop");
        lock l(sockets_mtx_);
        logger_sub_socket_.disconnect_all();
        // give a chance to resubscribe when new endpoints arrive
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return true;
      }
      
      zmq::message_t msg;
      bool has_monitors;
      {
        lock l(monitors_mtx_);
        has_monitors = !monitors_.empty();
      }
      
      if( logger_sub_socket_.get().recv(&msg) )
      {
        if( !msg.data() || !msg.size() )
          return true;
        
        // assume this is a proper pub message
        // so we don't need to care about the first part
        
        while( msg.more() )
        {
          msg.rebuild();
          logger_sub_socket_.get().recv(&msg);
          if( msg.data() && msg.size() && has_monitors )
          {
            try
            {
              pb::LogRecord logrec;
              bool parsed = logrec.ParseFromArray(msg.data(), msg.size());
              if( parsed )
              {
                lock l(monitors_mtx_);
                for( auto & m : monitors_ )
                {
                  try
                  {
                    m.second(m.first,logrec);
                  }
                  catch (...)
                  {
                    // don't care about monitor exceptions to avoid
                    // log message loops
                  }
                }
              }
            }
            catch (...)
            {
              // don't care about errors here
            }
          }
        }
      }
    }
    return true;
  }
  
  void
  log_record_client::get_logs(const interface::pb::GetLogs & req,
                              std::function<bool(interface::pb::LogRecord & rec)> fun)
  {
    {
      lock l(sockets_mtx_);
      if( !logger_req_socket_.valid() ) return;
    }
    
    zmq::message_t msg;
    int req_size = req.ByteSize();
    
    if( req_size > 0 )
    {
      util::flex_alloc<unsigned char, 256> buffer(req_size);
      
      bool serialized = req.SerializeToArray(buffer.get(), req_size);
      if( !serialized )
      {
        THROW_("Couldn't serialize GetLogs data");
      }
      
      logger_req_socket_.get().send( buffer.get(), req_size );
      
      bool call_fun = true;
      msg.rebuild();
      
      std::function<bool(zmq::message_t & msg)> process_logs{
        [&](zmq::message_t & msg) {

          if( msg.data() && msg.size() )
          {
            try
            {
              pb::LogRecord rec;
              serialized = rec.ParseFromArray(msg.data(), msg.size());
              if( serialized && call_fun )
              {
                call_fun = fun(rec);
              }
            }
            catch( const std::exception & e )
            {
              std::string text{e.what()};
              LOG_ERROR("exception caught" << text);
            }
            catch( ... )
            {
              LOG_ERROR("unknown exception caught when parsing LogRecord");
            }
          }
          return msg.more();
        }
      };
      
      while( true )
      {
        if( !logger_req_socket_.get().recv(&msg) )
          break;
        
        if( !process_logs(msg) )
          break;
        
        msg.rebuild();
      }
    }
  }
  
  void
  log_record_client::watch(const std::string & name, log_monitor m)
  {
    size_t orig_size = 0;
    {
      lock l(monitors_mtx_);
      orig_size = monitors_.size();
      auto it = monitors_.find(name);
      if( it != monitors_.end() )
        monitors_.erase(it);
    
      monitors_[name] = m;
      
      {
        lock l(sockets_mtx_);
        if( logger_sub_socket_.valid() )
        {
          if( !orig_size )
          {
            // we only subscribe if there is a watch and this is the first watch
            logger_sub_socket_.get().setsockopt(ZMQ_SUBSCRIBE, "*", 0);
          }
        }
      }
    }
  }
  
  void
  log_record_client::remove_watches()
  {
    {
      lock l(monitors_mtx_);
      if( !monitors_.empty() )
        monitors_.clear();
      
      {
        lock l(sockets_mtx_);
        if( logger_sub_socket_.valid() )
        {
          // we can unsubscribe
          logger_sub_socket_.get().setsockopt(ZMQ_UNSUBSCRIBE, "*", 0);
        }
      }
    }
  }
  
  void
  log_record_client::remove_watch(const std::string & name)
  {
    {
      lock l(monitors_mtx_);
      auto it = monitors_.find(name);
      if( it != monitors_.end() )
        monitors_.erase(it);
      
      {
        lock l(sockets_mtx_);
        if( logger_sub_socket_.valid() )
        {
          if( monitors_.empty() )
          {
            // we can unsubscribe if noone cares about zmq messages
            logger_sub_socket_.get().setsockopt(ZMQ_UNSUBSCRIBE, "*", 0);
          }
        }
      }
    }
  }

  bool
  log_record_client::on_endpoint_data(const interface::pb::EndpointData & ep)
  {
    bool no_change = true;
    
    std::function<bool(util::zmq_socket_wrapper & sock,
                       const std::string & addr)> connect_socket =
    {
      [this,&no_change](util::zmq_socket_wrapper & sock,
                        const std::string & addr)
      {
        bool ret = false;
        try
        {
          lock l(sockets_mtx_);
          sock.reconnect(addr.c_str());
          ret = true;
        }
        catch( const std::exception & e )
        {
          std::string text{e.what()};
          LOG_ERROR("exception in connect_socket lambda function" << V_(text));
        }
        catch( ... )
        {
          LOG_ERROR("unknown exception in connect_socket lambda function");
        }
        return ret;
      }
    };
    
    for( int i=0; i<ep.connections_size(); ++i )
    {
      auto conn = ep.connections(i);
      if(conn.type() == pb::ConnectionType::PUSH_PULL &&
         ep.svctype() == pb::ServiceType::LOG_RECORD)
      {
        // check if the configured address is still valid
        if( !logger_push_socket_->connected_to(conn.address().begin(), conn.address().end()) )
        {
          for( int ii=0; ii<conn.address_size(); ++ii )
          {
            if( connect_socket(*logger_push_socket_, conn.address(ii)) )
            {
              log_sink_sptr_.reset(new log_sink(logger_push_socket_));
              LOG_TRACE("logger configured" << V_(conn.address(ii)));
              no_change = false;
              break;
            }
          }
        }
      } // end PUSH_PULL
      
      if(conn.type() == pb::ConnectionType::PUB_SUB &&
         ep.svctype() == pb::ServiceType::LOG_RECORD)
      {
        if( !logger_sub_socket_.connected_to(conn.address().begin(), conn.address().end()) )
        {
          for( int ii=0; ii<conn.address_size(); ++ii )
          {
            if( connect_socket(logger_sub_socket_, conn.address(ii)) )
            {
              LOG_TRACE("log subscription configured" << V_(conn.address(ii)));
              no_change = false;
              break;
            }
          }
        }
      } // end PUB_SUB
      
      if(conn.type() == pb::ConnectionType::REQ_REP &&
         ep.svctype() == pb::ServiceType::GET_LOGS)
      {
        if( !logger_req_socket_.connected_to(conn.address().begin(), conn.address().end()) )
        {
          for( int ii=0; ii<conn.address_size(); ++ii )
          {
            if( connect_socket(logger_req_socket_, conn.address(ii)) )
            {
              LOG_TRACE("log REQ socket configured" << V_(conn.address(ii)));
              no_change = false;
              break;
            }
          }
        }
      } // end REQ_REP
      
    }
    return no_change;
  }
  
  log_record_client::~log_record_client()
  {
    worker_.stop();
  }
  
}}
