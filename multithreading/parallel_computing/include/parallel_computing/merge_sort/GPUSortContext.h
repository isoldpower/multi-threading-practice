#pragma once

#include <graphics/GraphicsBuffer.h>
#include <graphics/GraphicsDevice.h>
#include <graphics/GraphicsFactory.h>
#include <graphics/GraphicsKernel.h>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace multithreading::parallel_computing::merge_sort {

    template <typename T>
    struct GPUSortType;

    template <>
    struct GPUSortType<std::int32_t> {
        static constexpr std::string_view suffix = "int";
    };

    template <>
    struct GPUSortType<std::uint32_t> {
        static constexpr std::string_view suffix = "uint";
    };

    template <>
    struct GPUSortType<float> {
        static constexpr std::string_view suffix = "float";
    };

    template <typename T>
    concept GPUSortable = requires { GPUSortType<T>::suffix; };

    template <typename Compare, typename T>
    concept GPUSortOrder = std::same_as<Compare, std::less<T>> || std::same_as<Compare, std::less<>>
        || std::same_as<Compare, std::greater<T>> || std::same_as<Compare, std::greater<>>;

    template <typename Compare, typename T>
    inline constexpr bool sorts_descending =
        std::same_as<Compare, std::greater<T>> || std::same_as<Compare, std::greater<>>;

    inline constexpr std::size_t GPU_SORT_MAX_SIZE = std::size_t{1} << 30U;

    struct GPUSortParameters {  // NOLINT(altera-struct-pack-align)
        std::uint32_t first = 0;
        std::uint32_t second = 0;
        std::uint32_t count = 0;
        std::uint32_t descending = 0;
    };

    class GPUSortContext {
    public:
        explicit GPUSortContext(std::unique_ptr<graphics::GraphicsDevice> device)
            : graphics_device(std::move(device)) {
            if (!graphics_device) {
                throw std::invalid_argument("GPUSortContext requires a graphics device");
            }

            for (const std::string_view algorithm : KERNEL_ALGORITHMS) {
                for (const std::string_view type : KERNEL_TYPES) {
                    std::string name = kernel_name(algorithm, type);
                    auto loaded_kernel = graphics_device->load_kernel(name);
                    kernels.emplace(std::move(name), std::move(loaded_kernel));
                }
            }
        }

        static std::shared_ptr<GPUSortContext> create(const std::filesystem::path& shader_library) {
            auto device = graphics::GraphicsFactory::build_graphics(
                graphics::GraphicsOptions{.shader_library_path = shader_library});
            if (!device) {
                return nullptr;
            }

            return std::make_shared<GPUSortContext>(std::move(device));
        }

        [[nodiscard]] graphics::GraphicsDevice& device() const {
            return *graphics_device;
        }

        template <GPUSortable T>
        [[nodiscard]] const graphics::GraphicsKernel& kernel(const std::string_view algorithm) const {
            return *kernels.at(kernel_name(algorithm, GPUSortType<T>::suffix));
        }

        template <GPUSortable T>
        static void upload(
            const std::span<const T> values,
            graphics::GraphicsSharedBuffer& buffer,
            const std::size_t element_offset
        ) {
            const std::span<std::byte> destination =
                buffer.host_view().subspan(element_offset * sizeof(T), values.size_bytes());
            std::memcpy(destination.data(), values.data(), values.size_bytes());
        }

        template <GPUSortable T>
        static void download(graphics::GraphicsSharedBuffer& buffer, const std::span<T> values) {
            const std::span<const std::byte> source = buffer.host_view().first(values.size_bytes());
            std::memcpy(values.data(), source.data(), values.size_bytes());
        }

        static std::uint32_t checked_size(const std::size_t size) {
            if (size > GPU_SORT_MAX_SIZE) {
                throw std::length_error("Input is too large for the GPU sort kernels");
            }

            return static_cast<std::uint32_t>(size);
        }

    private:
        static constexpr std::array<std::string_view, 3> KERNEL_ALGORITHMS{
            "bitonic_step",
            "bitonic_local",
            "merge_pass",
        };
        static constexpr std::array<std::string_view, 3> KERNEL_TYPES{
            "int",
            "uint",
            "float",
        };

        std::unique_ptr<graphics::GraphicsDevice> graphics_device;
        std::unordered_map<std::string, std::unique_ptr<graphics::GraphicsKernel>> kernels;

        static std::string kernel_name(const std::string_view algorithm, const std::string_view type) {
            std::string name(algorithm, std::allocator<char>{});
            name += '_';
            name += type;
            return name;
        }
    };
}  // namespace multithreading::parallel_computing::merge_sort
