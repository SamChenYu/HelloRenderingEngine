////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "avocet/OpenGL/Resources/ResourceHandle.hpp"
#include "avocet/OpenGL/Resources/Labels.hpp"

#include <filesystem>
#include <unordered_map>
#include <span>

namespace avocet::opengl {
    template<class T>
    inline constexpr bool has_shader_lifecycle_events_v{
        requires(T& t, const GladGLContext& ctx, const resource_handle& handle) {
            { t.create(ctx) } -> std::same_as<resource_handle>;
            T::destroy(ctx, handle);
        }
    };

    template<class LifeEvents>
        requires has_shader_lifecycle_events_v<LifeEvents>
    class generic_shader_resource {
        const GladGLContext* m_Context;
        resource_handle m_Handle;
    public:
        template<class... Args>
            requires std::is_constructible_v<LifeEvents, Args...>
        explicit(sizeof...(Args) == 2) generic_shader_resource(const GladGLContext& ctx, const Args&... args)
            : m_Context{&ctx}
            , m_Handle{LifeEvents{args...}.create(ctx)}
        {}

        ~generic_shader_resource() { if(m_Context) LifeEvents::destroy(*m_Context, m_Handle); }

        generic_shader_resource(generic_shader_resource&&) noexcept = default;

        generic_shader_resource& operator=(generic_shader_resource&&) noexcept = default;

        [[nodiscard]]
        const resource_handle& handle() const noexcept { return m_Handle; }

        [[nodiscard]]
        friend bool operator==(const generic_shader_resource&, const generic_shader_resource&) noexcept = default;
    };


    template<class LifeEvents>
    [[nodiscard]]
    GLuint get_index(const generic_shader_resource<LifeEvents>& gsr) noexcept { return get_index(gsr.handle()); }

    struct shader_program_resource_lifecycle {
        [[nodiscard]]
        static resource_handle create(const GladGLContext& ctx) { return resource_handle{gl_function{&GladGLContext::CreateProgram}(ctx)}; }

        static void destroy(const GladGLContext& ctx, const resource_handle& handle) { gl_function{&GladGLContext::DeleteProgram}(ctx, get_index(handle)); }
    };

    using shader_program_resource = generic_shader_resource<shader_program_resource_lifecycle>;

    struct string_hash {
        using is_transparent = void;

        [[nodiscard]]
        std::size_t operator()(const std::string& txt) const noexcept {
            return std::hash<std::string>{}(txt);
        }

        [[nodiscard]]
        std::size_t operator()(std::string_view txt) const noexcept {
            return std::hash<std::string_view>{}(txt);
        }
    };

    class shader_program {
    public:
        shader_program(const GladGLContext& ctx, const std::filesystem::path& vertexShaderSource, const std::filesystem::path& fragmentShaderSource);

        shader_program(shader_program&&) noexcept = default;

        shader_program& operator=(shader_program&&) noexcept = default;

        ~shader_program() { if(m_Context) program_tracker::reset(*m_Context, m_Resource); }

        [[nodiscard]]
        std::string extract_label() const { return get_object_label(*m_Context, object_identifier::program, m_Resource.handle()); }

        void use(const GladGLContext& ctx) { program_tracker::utilize(ctx, m_Resource); }

        void set_uniform(const GladGLContext& ctx, std::string_view name, GLfloat val) {
            do_set_uniform(ctx, name, gl_function{&GladGLContext::Uniform1f}, val);
        }

        void set_uniform(const GladGLContext& ctx, std::string_view name, GLint val) {
            do_set_uniform(ctx, name, gl_function{&GladGLContext::Uniform1i}, val);
        }

        void set_uniform(const GladGLContext& ctx, std::string_view name, std::span<const GLfloat, 2> vals) {
            do_set_uniform(ctx, name, gl_function{&GladGLContext::Uniform2f}, vals[0], vals[1]);
        }

        [[nodiscard]]
        friend bool operator==(const shader_program&, const shader_program&) noexcept = default;
    private:
        const GladGLContext* m_Context;
        shader_program_resource m_Resource;
        using map_t = std::unordered_map<std::string, GLint, string_hash, std::ranges::equal_to>;
        map_t m_Uniforms;

        [[nodiscard]]
        GLint extract_uniform_location(const GladGLContext& ctx, std::string_view name);

        template<class... Args>
        void do_set_uniform(const GladGLContext& ctx, std::string_view name, gl_function<void(const GladGLContext&, GLint, Args...)> fn, Args... args) {
            use(ctx);
            fn(ctx, extract_uniform_location(ctx, name), args...);
        }

        class program_tracker {
            inline static GLuint st_Current{};
        public:
            static void utilize(const GladGLContext& ctx, const shader_program_resource& spr) {
                if(const auto index{get_index(spr)}; index != st_Current) {
                    gl_function{&GladGLContext::UseProgram}(ctx, index);
                    st_Current = index;
                }
            }

            static void reset(const GladGLContext& ctx, const shader_program_resource& spr) {
                if(get_index(spr) == st_Current)
                    st_Current = 0;
            }
        };
    };
}