#pragma once

#include <utility>

template <typename Quantity, typename QuantityFecherFunc, typename NotifierFunc>
struct scoped_change_notifier
{
   scoped_change_notifier(QuantityFecherFunc&& fetcher, NotifierFunc&& notifier) noexcept : m_fetcher(std::move(fetcher)), m_quantity(m_fetcher()), m_notifier(std::move(notifier))
   {
   }

   scoped_change_notifier(const scoped_change_notifier&) = delete;
   scoped_change_notifier& operator=(const scoped_change_notifier&) = delete;
   scoped_change_notifier(scoped_change_notifier&&) = default;
   scoped_change_notifier& operator=(scoped_change_notifier&&) = default;

   ~scoped_change_notifier() noexcept
   {
      if (m_quantity != m_fetcher())
         m_notifier();
   }

private:
   QuantityFecherFunc m_fetcher;
   Quantity m_quantity;
   NotifierFunc m_notifier;
};
