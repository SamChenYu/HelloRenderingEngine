////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

/*! \file */

#include "NullFunctionPointerFreeTest.hpp"

#include "curlew/Window/GLFWWrappers.hpp"
#include "avocet/OpenGL/Utilities/GLFunction.hpp"

#include "glad/gl.h"

namespace avocet::testing
{
    [[nodiscard]]
    std::filesystem::path null_function_pointer_free_test::source_file() const
    {
        return std::source_location::current().file_name();
    }

    void null_function_pointer_free_test::run_tests()
    {
        namespace agl = avocet::opengl;
        using namespace curlew;

        glfw_manager manager{};
        auto w{manager.create_window({.hiding{window_hiding_mode::on}})};
        const auto& ctx = w.gl_context();

        check_exception_thrown<std::runtime_error>(
            "Constructing gl_function with a null pointer",
            [&ctx](){
                auto& mutable_ctx = const_cast<GladGLContext&>(ctx);
                gl_breaker breaker{mutable_ctx.GetError};
                return agl::gl_function{agl::unchecked_debug_output, &GladGLContext::GetError}(ctx);
            }
        );

        check_exception_thrown<std::runtime_error>(
            "Null GetError when checking for basic errors",
            [&ctx](){
                auto& mutable_ctx = const_cast<GladGLContext&>(ctx);
                gl_breaker breaker{mutable_ctx.GetError};
                agl::check_for_basic_errors(agl::num_messages{10}, std::source_location::current());
            }
        );

        check_exception_thrown<std::runtime_error>(
            "Null BindBuffer",
            [&ctx](){
                auto& mutable_ctx = const_cast<GladGLContext&>(ctx);
                gl_breaker breaker{mutable_ctx.BindBuffer};
                agl::gl_function{&GladGLContext::BindBuffer}(ctx, GL_ARRAY_BUFFER, 42);
            }
        );
    }
}
