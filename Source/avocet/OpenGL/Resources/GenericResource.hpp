////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "avocet/OpenGL/Resources/ResourceHandle.hpp"
#include "avocet/OpenGL/Utilities/ObjectIdentifiers.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>
#include <span>

namespace avocet::opengl {
    struct num_resources { std::size_t value{}; };

    template<std::size_t I>
    struct index { constexpr static std::size_t value{I}; };

    template<num_resources NumResources, class T>
    inline constexpr bool has_resource_lifecycle_events_v{
        requires(const GladGLContext& ctx, raw_indices<NumResources.value>& indices, const resource_handle& h) {
            T::generate(ctx, indices);
            T::destroy(ctx, indices);
            { T::identifier } -> std::convertible_to<object_identifier>;
            T::bind(ctx, h);
            typename T::configurator;
            T::configure(ctx, h, std::declval<typename T::configurator>());
        }
    };

    template<num_resources NumResources, class LifeEvents>
        requires has_resource_lifecycle_events_v<NumResources, LifeEvents>
    struct resource_lifecycle {
        using configurator_type = LifeEvents::configurator;

        constexpr static std::size_t N{NumResources.value};

        [[nodiscard]]
        static handles<N> generate(const GladGLContext& ctx) {
            raw_indices<N> indices{};
            LifeEvents::generate(ctx, indices);
            return to_handles(indices);
        }

        static void destroy(const GladGLContext& ctx, const handles<N>& h) {
            LifeEvents::destroy(ctx, to_raw_indices(h));
        }

        static void bind(const GladGLContext& ctx, const resource_handle& h) { LifeEvents::bind(ctx, h); }

        static void configure(const GladGLContext& ctx, const resource_handle& h, const configurator_type& config) {
            LifeEvents::configure(ctx, h, config);
        }
    };

    template<num_resources NumResources, class LifeEvents>
        requires has_resource_lifecycle_events_v<NumResources, LifeEvents>
    class resource_wrapper{
    public:
        using lifecycle_type = resource_lifecycle<NumResources, LifeEvents>;

        constexpr static std::size_t N{NumResources.value};

        resource_wrapper(const GladGLContext& ctx) : m_Context{&ctx}, m_Handles{lifecycle_type::generate(ctx)} {}
        ~resource_wrapper() { if(m_Context) lifecycle_type::destroy(*m_Context, m_Handles); }

        resource_wrapper(resource_wrapper&&)           noexcept = default;
        resource_wrapper& operator=(resource_wrapper&&) noexcept = default;

        [[nodiscard]]
        const handles<N>& get_handles() const noexcept { return m_Handles; }

        [[nodiscard]]
        friend bool operator==(const resource_wrapper&, const resource_wrapper&) noexcept = default;
    private:
        const GladGLContext* m_Context;
        handles<N> m_Handles;
    };

    template<num_resources NumResources, class LifeEvents>
        requires (NumResources.value > 0) && has_resource_lifecycle_events_v<NumResources, LifeEvents>
    class generic_resource {
        using resource_type = resource_wrapper<NumResources, LifeEvents>;
        using lifecycle_type = resource_type::lifecycle_type;
        const GladGLContext* m_Context;
        resource_type m_Resource;
    public:
        using configurator_type = lifecycle_type::configurator_type;
        constexpr static std::size_t N{NumResources.value};

        explicit generic_resource(const GladGLContext& ctx, const std::array<configurator_type, N>& configs)
            : m_Context{&ctx}
            , m_Resource{ctx}
        {
            for(const auto& [handle, config] : std::views::zip(get_handles(), configs)) {
                if(handle == resource_handle{})
                    throw std::runtime_error{"generic_resource  - null resource"};

                lifecycle_type::bind(ctx, handle);
                lifecycle_type::configure(ctx, handle, config);
            }
        }

        [[nodiscard]]
        bool is_null() const noexcept {
            auto isNull{[](const resource_handle& h){ return h == resource_handle{}; }};
            assert(std::ranges::all_of(get_handles(), isNull) or std::ranges::none_of(get_handles(), isNull));
            return isNull(get_handle(index<0>{}));
        }

        template<std::size_t I>
            requires (I < N)
        [[nodiscard]]
        std::string extract_label(index<I> i) const { return get_object_label(*m_Context, LifeEvents::identifier, get_handle(i)); }

        [[nodiscard]]
        std::string extract_label() const requires (N == 1) { return extract_label(index<0>{}); }

        [[nodiscard]]
        friend bool operator==(const generic_resource&, const generic_resource&) noexcept = default;
    protected:
        ~generic_resource() = default;

        generic_resource(generic_resource&&)            noexcept = default;
        generic_resource& operator=(generic_resource&&) noexcept = default;

        template<std::size_t I>
            requires (I < N)
        static void do_bind(const GladGLContext& ctx, const generic_resource& gbo, index<I> i) { lifecycle_type::bind(ctx, gbo.get_handle(i)); }

        static void do_bind(const GladGLContext& ctx, const generic_resource& gbo) requires (N == 1) { do_bind(ctx, gbo, index<0>{}); }
    private:
        [[nodiscard]]
        const handles<N>& get_handles() const noexcept { return m_Resource.get_handles(); }

        template<std::size_t I>
            requires (I < N)
        const resource_handle& get_handle(index<I>) const noexcept { return get_handles()[I]; }
    };
}