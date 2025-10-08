////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "avocet/OpenGL/Debugging/Errors.hpp"

#include <glad/gl.h>

#include <concepts>
#include <format>
#include <stdexcept>

namespace avocet::opengl {
    struct unchecked_debug_output_t {};

    inline constexpr unchecked_debug_output_t unchecked_debug_output{};

    template<class, debugging_mode Mode=inferred_debugging_mode()> class gl_function;

    template<class R, class... Args, debugging_mode Mode>
    class [[nodiscard]] gl_function<R(Args...), Mode> {
    public:
        using function_pointer_type = R(*)(Args...);

        constexpr static num_messages max_reported_messages{10};

        constexpr gl_function(function_pointer_type GladGLContext::* fnPtr, [[maybe_unused]] std::source_location loc = std::source_location::current())
            : m_FnMemberPtr{fnPtr}
        {}

        constexpr gl_function(unchecked_debug_output_t, function_pointer_type GladGLContext::* fnPtr, [[maybe_unused]] std::source_location loc = std::source_location::current())
            : m_FnMemberPtr{fnPtr}
        {
            static_assert(Mode == debugging_mode::none);
        }

        [[nodiscard]]
        R operator()(const GladGLContext& ctx, Args... args, std::source_location loc = std::source_location::current()) const {
            auto fn = ctx.*m_FnMemberPtr;
            if(!fn) throw std::runtime_error{std::format("gl_function: null function pointer at {}", to_string(loc))};

            const auto ret{fn(args...)};
            check_for_errors(ctx, loc);
            return ret;
        }

        void operator()(const GladGLContext& ctx, Args... args, std::source_location loc = std::source_location::current()) const
            requires std::is_void_v<R>
        {
            auto fn = ctx.*m_FnMemberPtr;
            if(!fn) throw std::runtime_error{std::format("gl_function: null function pointer at {}", to_string(loc))};

            fn(args...);
            check_for_errors(ctx, loc);
        }
    private:
        function_pointer_type GladGLContext::* m_FnMemberPtr;

        static void check_for_errors(const GladGLContext& ctx, std::source_location loc) {
            if constexpr(Mode != debugging_mode::none) {
                check_for_basic_errors(ctx, max_reported_messages, loc);
            }
        }
    };

    template<class R, class... Args>
    gl_function(R(GladGLContext::*)(Args...)) -> gl_function<R(Args...)>;

    template<class R, class... Args>
    gl_function(R(*GladGLContext::*)(Args...)) -> gl_function<R(Args...)>;

    template<class R, class...Args>
    gl_function(unchecked_debug_output_t, R(GladGLContext::*)(Args...)) -> gl_function<R(Args...), debugging_mode::none>;

    template<class R, class...Args>
    gl_function(unchecked_debug_output_t, R(*GladGLContext::*)(Args...)) -> gl_function<R(Args...), debugging_mode::none>;

    template<class R, class...Args>
    gl_function(unchecked_debug_output_t, R(GladGLContext::*)(Args...), std::source_location) -> gl_function<R(Args...), debugging_mode::none>;

    template<class R, class...Args>
    gl_function(unchecked_debug_output_t, R(*GladGLContext::*)(Args...), std::source_location) -> gl_function<R(Args...), debugging_mode::none>;
}