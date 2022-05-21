#ifndef _HAKO_CONTEXT_HPP_
#define _HAKO_CONTEXT_HPP_

#include <sys/types.h>
#include <unistd.h>

namespace hako::core::context {
    class HakoContext {
    public:
        HakoContext()
        {
            this->pid_ = getpid();
        }

        pid_t get_pid()
        {
            return this->pid_;
        }

        bool is_same(HakoContext &context)
        {
            return (context.get_pid() == this->get_pid());
        }
        bool is_same(pid_t pid)
        {
            return (this->get_pid() == pid);
        }

        virtual ~HakoContext() {}
    
    private:
        pid_t pid_;
    };
}

#endif /* _HAKO_CONTEXT_HPP_ */