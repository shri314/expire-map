#include "expire_map.h"

#include <cassert>
#include <string>

using namespace std::chrono_literals;

inline void unit_test()
{
   for (int i = 0; i < 5; ++i)
   {
      auto start = std::chrono::system_clock::now();

      {
         expire_map<int, std::string> M;
         assert(M.size() == 0);
         assert(M.get(10).value_or("xx") == "xx");
         assert(M.get(11).value_or("xx") == "xx");
         assert(M.get(12).value_or("xx") == "xx");
         assert(M.get(13).value_or("xx") == "xx");

         M.emplace(10, "10", 50);
         assert(M.size() == 1);
         assert(M.get(10).value_or("xx") == "10");   // exp: 50ms
         assert(M.get(11).value_or("xx") == "xx");
         assert(M.get(12).value_or("xx") == "xx");
         assert(M.get(13).value_or("xx") == "xx");

         std::this_thread::sleep_for(20ms);   // -20ms
         assert(M.size() == 1);
         assert(M.get(10).value_or("xx") == "10");   // exp: 30ms
         assert(M.get(11).value_or("xx") == "xx");
         assert(M.get(12).value_or("xx") == "xx");
         assert(M.get(13).value_or("xx") == "xx");

         M.emplace(11, "11", 500);   // new higher expirty - 500ms
         assert(M.size() == 2);
         assert(M.get(10).value_or("xx") == "10");   // exp: 30ms
         assert(M.get(11).value_or("xx") == "11");   // exp: 500ms
         assert(M.get(12).value_or("xx") == "xx");
         assert(M.get(13).value_or("xx") == "xx");

         std::this_thread::sleep_for(20ms);   // -20ms
         M.emplace(10, "10+", 50);
         assert(M.size() == 2);
         assert(M.get(10).value_or("xx") == "10+");   // exp: 50ms (exp reset from ~10ms)
         assert(M.get(11).value_or("xx") == "11");    // exp: 430ms
         assert(M.get(12).value_or("xx") == "xx");
         assert(M.get(13).value_or("xx") == "xx");

         std::this_thread::sleep_for(45ms);   // -45ms
         assert(M.size() == 2);
         assert(M.get(10).value_or("xx") == "10+");   // exp: 5ms
         assert(M.get(11).value_or("xx") == "11");    // exp: 385ms
         assert(M.get(12).value_or("xx") == "xx");
         assert(M.get(13).value_or("xx") == "xx");

         std::this_thread::sleep_for(10ms);   // -10ms
         assert(M.size() == 1);
         assert(M.get(10).value_or("xx") == "xx");   // exp: just now!
         assert(M.get(11).value_or("xx") == "11");   // exp: 375ms
         assert(M.get(12).value_or("xx") == "xx");
         assert(M.get(13).value_or("xx") == "xx");

         std::this_thread::sleep_for(50ms);   // -50ms
         assert(M.size() == 1);
         assert(M.get(10).value_or("xx") == "xx");   // (exp)
         assert(M.get(11).value_or("xx") == "11");   // exp: 320ms
         assert(M.get(12).value_or("xx") == "xx");
         assert(M.get(13).value_or("xx") == "xx");

         M.emplace(12, "12", 20);   // new lowest expiry - 20ms
         M.emplace(13, "13", 20);   // additional entry  - 20ms
         assert(M.size() == 3);
         assert(M.get(10).value_or("xx") == "xx");   // (exp)
         assert(M.get(11).value_or("xx") == "11");   // exp: 320ms
         assert(M.get(12).value_or("xx") == "12");   // exp: 20ms
         assert(M.get(13).value_or("xx") == "13");   // exp: 20ms

         std::this_thread::sleep_for(25ms);   // -25ms
         assert(M.size() == 1);
         assert(M.get(10).value_or("xx") == "xx");   // (exp)
         assert(M.get(11).value_or("xx") == "11");   // exp: 295ms
         assert(M.get(12).value_or("xx") == "xx");   // exp: just now!
         assert(M.get(13).value_or("xx") == "xx");   // exp: just now!

         // fall off the scope expiring everything instantly - without blocking in destructor for 295ms
      }

      auto end = std::chrono::system_clock::now();

      assert(end - start < 200ms);
   }
}

int main()
{
   unit_test();
}
