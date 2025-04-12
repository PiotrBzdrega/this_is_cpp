/* std::lock_guard
released when goes out of scope. */

/* std::unique_lock
std::lock_guard + manual unlocking + use with a condition_variable */

/* std::scoped_lock
>1 mutexes */


// why wait_and_pop uses unique_lock and try_pop uses lock_guard

// void wait_and_pop(T& value)
//  {
//  std::unique_lock<std::mutex> lk(mut);
//  data_cond.wait(lk,[this]{return !data_queue.empty();});
//  value=data_queue.front();
//  data_queue.pop();
//  }


// bool try_pop(T& value)
//  {
//  std::lock_guard<std::mutex> lk(mut);
//  if(data_queue.empty())
//  return false;
//  value=data_queue.front();
//  data_queue.pop();
//  return true;
//  } 

/* 
 * - unique_lock can manually lock/unlock (lock_guard cannot)
 *   it is needed to release lock when thread is waiting and 
 *   re-acuires lock when thread is notified
 * - lock_guard does not work with condition variables, 
 *   that are needed for wait()
 *
*/


/* lock vs scoped_lock (lock two mutexes simultaneusly)
void swap(X& lhs, X& rhs)
 {
 if(&lhs==&rhs)
 return;
 std::lock(lhs.m,rhs.m); 
 std::lock_guard<std::mutex> lock_a(lhs.m,std::adopt_lock); 
 std::lock_guard<std::mutex> lock_b(rhs.m,std::adopt_lock); 
 swap(lhs.some_detail,rhs.some_detail);
 }

 vs.

 void swap(X& lhs, X& rhs)
 {
 if(&lhs==&rhs)
 return;
 std::scoped_lock guard(lhs.m,rhs.m); 
 swap(lhs.some_detail,rhs.some_detail);
 }



*/