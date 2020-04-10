#include "src/Net485WorkQueue.hpp"

Net485WorkQueue queue = Net485WorkQueue(NULL);

void setup() {
  Serial.begin(115200);
}

void loop() {
    Net485Packet a, b, c, d, e;
    a.dataSize = 0;
    b.dataSize = 1;
    c.dataSize = 2;
    d.dataSize = 3;
    e.dataSize = 4;
    queue.pushWork(a);
    queue.pushWork(b);
    queue.pushWork(c);
    queue.pushWork(d);
    queue.pushWork(e);
    while(queue.hasWork()) {
        Serial.print(" Work: "); Serial.println(queue._next);
        int v = queue.doWork();
        Serial.print("Remaining work: "); Serial.println(v);
    }
    for (;;);
}
