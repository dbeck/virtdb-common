#pragma once

#include <data.pb.h>
#include <util/table_collector.hh>
#include <util/active_queue.hh>
#include <util/value_type_reader.hh>
#include <memory>
#include <vector>
#include <mutex>

namespace virtdb { namespace engine {
  
  class collector final
  {
  public:
    typedef std::shared_ptr<collector>                 sptr;
    typedef std::shared_ptr<interface::pb::Column>     column_sptr;
    typedef util::value_type_reader::sptr              reader_sptr;
    typedef std::vector<reader_sptr>                   reader_sptr_vec;
    typedef std::unique_lock<std::mutex>               lock;
    
  private:
    struct item
    {
      typedef std::shared_ptr<item> sptr;
      column_sptr    col_;
      reader_sptr    reader_;
      size_t         block_id_;
      size_t         col_id_;
    };
    
    typedef util::table_collector<item>         collector_t;
    typedef util::active_queue<item::sptr,50>   process_queue_t;

    collector_t            collector_;
    process_queue_t        queue_;
    int64_t                max_block_id_;
    int64_t                last_block_id_;
    mutable std::mutex     mtx_;
    
    collector() = delete;
    collector(const collector &) = delete;
    collector & operator=(const collector &) = delete;
    
    void
    prrocess(item::sptr itm);
    
  public:
    void push(size_t block_id,
              size_t col_id,
              column_sptr data);
    
    bool process(size_t block_id,
                 uint64_t timeout_ms,
                 bool wait_ready);
    
    bool get(size_t block_id,
             reader_sptr_vec & rdrs,
             uint64_t timeout_ms);
    
    void erase(size_t block_id);
    
    int64_t max_block_id() const;
    int64_t last_block_id() const;
    size_t n_columns() const;
    size_t n_queued() const;
    size_t n_done() const;
    
    collector(size_t n_cols);
    virtual ~collector();
  };
  
}}
