#pragma once

#include "util.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <experimental/optional>

template <typename key_t, typename value_t,
          template <typename, typename> typename alias_map = std::unordered_map /* values: std::unordered_map, std::map, or with custom allocator */,
          template <typename, typename> typename alias_multimap = std::multimap /* values: std::multimap, or with custom allocator */
          >
class expire_map
{
   struct dict_entry_t;
   struct expiry_entry_t;

   using time_point_t = std::chrono::time_point<std::chrono::system_clock>;
   using dictionary_t = alias_map<key_t, dict_entry_t>;
   using expiry_info_t = alias_multimap<time_point_t, expiry_entry_t>;

   struct dict_entry_t
   {
      value_t m_value;
      typename expiry_info_t::const_iterator m_expiry_ref;
   };

   struct expiry_entry_t
   {
      typename dictionary_t::const_iterator m_dictionary_ref;
   };

public:
   expire_map()
   {
      m_cleaner = std::thread([self = this]() {
         while (!self->m_exit)
         {
            std::unique_lock<std::mutex> L{self->m_mutex};
            if (time_point_t t = self->lowest_time(); t != time_point_t())
            {
               if (self->m_cv.wait_until(L, t) == std::cv_status::timeout)
               {
                  self->prune();
               }
            }
            else
            {
               self->m_cv.wait(L);
            }
         }
      });
   }

   ~expire_map()
   {
      m_exit = true;
      m_cv.notify_all();
      m_cleaner.join();
   }

   auto size() const
   {
      // [[expects: LOCK.owns_lock() == false]]

      auto LOCK = std::scoped_lock{m_mutex};

      return m_dictionary.size();
   }

   std::experimental::optional<value_t> get(key_t key) const
   {
      // [[expects: LOCK.owns_lock() == false]]

      auto LOCK = std::scoped_lock{m_mutex};

      if (auto i = m_dictionary.find(key); i != m_dictionary.end())
      {
         return i->second.m_value;
      }

      return {};
   }

   void emplace(key_t key, value_t value, long timeoutMs)
   {
      // [[expects: LOCK.owns_lock() == false]]

      auto LOCK = std::scoped_lock{m_mutex};
      auto NOTI = make_prune_time_change_notifier();

      auto e_hint = m_expiry_info.end();
      auto d_hint = m_dictionary.find(key);

      if (d_hint != m_dictionary.end())
      {
         // erase entry first (to allow an overwrite later)

         e_hint = m_expiry_info.erase(d_hint->second.m_expiry_ref);
         d_hint = m_dictionary.erase(d_hint);
      }

      auto d_ref = m_dictionary.emplace_hint(d_hint, std::move(key), dict_entry_t{std::move(value), {}});
      auto e_ref = m_expiry_info.emplace_hint(e_hint, std::chrono::system_clock::now() + std::chrono::milliseconds(timeoutMs), expiry_entry_t{d_ref});
      d_ref->second.m_expiry_ref = e_ref;
   }

   void erase(key_t key)
   {
      // [[expects: LOCK.owns_lock() == false]]

      auto LOCK = std::scoped_lock{m_mutex};
      auto NOTI = make_prune_time_change_notifier();

      if (auto i = m_dictionary.find(key); i != m_dictionary.end())
      {
         m_expiry_info.erase(i->second.m_expiry_ref);
         m_dictionary.erase(i);
      }
   }

private:
   auto make_prune_time_change_notifier()
   {
      // [[expects: LOCK.owns_lock() == true]]

      auto qfunc = [self = this]()
      {
         return self->lowest_time();
      };

      auto nfunc = [self = this]()
      {
         self->m_cv.notify_all();
      };

      return scoped_change_notifier<time_point_t, decltype(qfunc), decltype(nfunc)>{std::move(qfunc), std::move(nfunc)};
   }

   time_point_t lowest_time() const
   {
      // [[expects: LOCK.owns_lock() == true]]

      if (m_expiry_info.begin() != m_expiry_info.end())
         return m_expiry_info.begin()->first;

      return {};
   }

   void prune()
   {
      // [[expects: LOCK.owns_lock() == true]]

      auto del_end = m_expiry_info.upper_bound(std::chrono::system_clock::now());

      bool del = false;
      for (auto i = m_expiry_info.begin(); i != del_end; ++i)
      {
         m_dictionary.erase(i->second.m_dictionary_ref);
         del = true;
      }

      if (del)
         m_expiry_info.erase(m_expiry_info.begin(), del_end);
   }

private:
   std::atomic<bool> m_exit = false;

   dictionary_t m_dictionary;     // FIXME: enfore at compile time that this is accessed only with lock held
   expiry_info_t m_expiry_info;   // FIXME: enfore at compile time that this is accessed only with lock held
   mutable std::mutex m_mutex;
   std::condition_variable m_cv;

   std::thread m_cleaner;   // FIXME: requires one thread per unique template instance!!
                            //      : provide a way to opt out
                            //      : or make a common cleaner thread for all instances
};
