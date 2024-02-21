#ifndef _HAKO_CONTEXT_HPP_
#define _HAKO_CONTEXT_HPP_

#include "types/hako_types.hpp"

namespace hako::core::context {
    class HakoContext {
    public:
        HakoContext()
        {
#ifdef _WIN32
            this->pid_ = GetCurrentProcessId();
#else
            this->pid_ = getpid();
#endif
        }

        pid_type get_pid()
        {
            return this->pid_;
        }

        bool is_same(HakoContext &context)
        {
            return (context.get_pid() == this->get_pid());
        }
        bool is_same(pid_type pid)
        {
            return (this->get_pid() == pid);
        }

        virtual ~HakoContext() {}
    
    private:
        pid_type pid_;
    };
}

#endif /* _HAKO_CONTEXT_HPP_ */