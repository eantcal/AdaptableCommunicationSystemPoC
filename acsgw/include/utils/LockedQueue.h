//  
// This file is part of acsgw
// Copyright (c) Antonino Calderone (antonino.calderone@gmail.com)
// All rights reserved.  
// Licensed under the MIT License. 
// See COPYING file in the project root for full license information.
//

#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

/* -------------------------------------------------------------------------- */

template <class T>
class LockedQueue
{
public:
   LockedQueue(int maxSize = 1) : _maxSize(maxSize) {}

   bool push(T &&item)
   {
      std::unique_lock<std::mutex> lk(_lock);

      if (_data.size() >= _maxSize)
      {
         return false;
      }

      _data.push(std::move(item));

      lk.unlock();
      _cv.notify_one();

      return true;
   }

   // return false either if a timeout occours or timeout=0 and there is no data ready 
   bool pop(T &res, int timeout, std::function<bool()> cond = [] { return false; })
   {
      std::unique_lock<std::mutex> lk(_lock);

      bool bTimeout = false;

      if (_data.empty())
      {
         if (timeout == 0)
         {
            return false;
         }

         if (timeout > 0)
         {
            std::function<bool()> pred = [&]() { return cond() || !_data.empty(); };
            using namespace std::chrono_literals;
            bTimeout = !_cv.wait_for(lk, timeout * 1ms, pred);
         }
         else if (timeout < 0)
         {
            std::function<bool()> pred = [&]() { return cond() || !_data.empty(); };
            _cv.wait(lk, pred);
         }
      }

      if (bTimeout || _data.empty())
      {
         return false;
      }

      res = _data.front();
      _data.pop();

      return true;
   }

private:
   int _maxSize = 1;
   std::mutex _lock;
   std::condition_variable _cv;
   std::queue<T> _data;
};
