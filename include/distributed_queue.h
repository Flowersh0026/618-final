#ifndef _DISTRIBUTED_QUEUE_H_
#define _DISTRIBUTED_QUEUE_H_

#include "queue.h"
#include <vector>
#include <atomic>
#include <mutex>

using namespace std;

template<class T>
class DistributedQueue : public Queue<T> {
 public:
  // Default size 8
  DistributedQueue() : is_master_(true), policy_(0), size_(8) {
    CreateMaster();
  }
  DistributedQueue(int size) : is_master_(true), policy_(0), size_(size) {
    CreateMaster();
  }
  DistributedQueue(int size, bool is_node) : is_master_(false), policy_(0), size_(size) {
    if (!is_node) return;
    q_ = new RtmQueue<T>();
  }

  ~DistributedQueue() {
    if (!is_master_) {
      delete q_;
    } else {

      // Shutdown all nodes
      for (int i = 1; i < size_; ++i) {
        delete queues_[i];
      }
    }
  }

  virtual void Push(const T& value) {
    lock_guard<mutex> lock(mu_);
    if (!is_master_) {
      return q_->Push(value);
    }
    int nodeID = get_push_id();
    return queues_[nodeID]->Push(value);
  }

  virtual void Push(T&& value) {
    lock_guard<mutex> lock(mu_);
    if (!is_master_) {
      return q_->Push(value);
    }
    int nodeID = get_push_id();
    return queues_[nodeID]->Push(value);
  }

  virtual std::optional<T> Pop() {
    lock_guard<mutex> lock(mu_);
    if (!is_master_) {
      return q_->Pop();
    }
    int nodeID = get_pop_id();
    return queues_[nodeID]->Pop();
  }

  void SetNodeList(vector<DistributedQueue<T>*> list, int id) {
    id_ = id;
    queues_ = vector<DistributedQueue<T>*>(list);
  } 

 private:
  bool is_master_;    // Current server is master server
  int policy_;        // By default RoundRobin
  int size_;          // Number of queues
  mutex mu_;          // Protection lock

  /* Only valid for master node */
  int count_ = 0;         // Current number of elements in queue
  int next_push_id_ = 0;  // The id which node should push to
  int next_pop_id_ = 0;   // The id which node should push to

  /* Only valid for normal nodes (other than master) */
  int id_ = -1;               // Server ID
  Queue<T>* q_ = nullptr;     // The underlying queue storing values


  // List of node (queue) in this distributed queue system
  // Note: does not include master node
  vector<DistributedQueue<T>*> queues_;

  void CreateMaster() {
    lock_guard<mutex> lock(mu_);
    for (int i = 0; i < size_; ++i) {
      queues_.push_back(CreateNode());
    }
    for (int i = 0; i < size_; ++i) {
      queues_[i]->SetNodeList(queues_, i);
    }
  }

  DistributedQueue* CreateNode() {
    return new DistributedQueue<T>(size_, true);
  }

  // According to policy, returns server id that should push into
  int get_push_id() {
    int retval = next_push_id_;
    next_push_id_ = (next_push_id_ + 1) % size_;
    return retval;
  }

  // According to policy, returns server id that should pop from
  int get_pop_id() {
    int retval = next_pop_id_;
    next_pop_id_ = (next_pop_id_ + 1) % size_;
    return retval;
  }

};

#endif  // _DISTRIBUTED_QUEUE_H_