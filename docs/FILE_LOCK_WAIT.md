# Bounded file-lock waits

The historical file-lock APIs remain blocking by default. This preserves the
runtime behavior of existing masters, assets, and conductors.

Command-line or diagnostic callers that must fail in bounded time can create a
`HakoFlockWaitScope` on the current thread:

```cpp
volatile sig_atomic_t interrupted = 0;
HakoFlockWaitScope scope({3000, &interrupted});
```

While the scope is active, both operating-system file-lock acquisition and the
file-backed logical semaphore wait use a monotonic deadline. They throw
`HakoFlockWaitError` on timeout, interruption, or a system error. A failed
acquisition never increments `HakoFlockObjectType::lock_count`.

The signal handler must only store the received signal in the supplied
`sig_atomic_t`. It must not throw, log, or modify lock state. Waiting code checks
the flag only while it does not own the file lock, so interruption cannot leave
an acquired file lock behind.

The scope is thread-local. Callers that do not create it continue to use the
existing indefinite-wait behavior.
