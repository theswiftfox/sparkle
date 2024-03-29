/**
*	Copyright (c) 2017 Patrick Gantner
*
*   This work is licensed under the Creative Commons Attribution 4.0 International License.
*	To view a copy of this license, visit http://creativecommons.org/licenses/by/4.0/
*	or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.
*
**/

#ifndef VULKAN_EXTENSION_H
#define VULKAN_EXTENSION_H
#include <vulkan/vulkan.h>

#include <cassert>
#include <cstring>
#include <iostream>

#define VK_THROW_ON_ERROR(f, msg)				\
{												\
	if ((f) != VK_SUCCESS) {					\
		auto message = std::string(__func__);	\
		message += " : ";						\
		message += (msg);						\
		throw std::runtime_error(message);		\
	}											\
}			


namespace vkExt {
	struct SharedMemory {
		VkDevice device;
		VkDeviceMemory memory = nullptr;
		VkDeviceSize size = 0;
		void* mapped = nullptr;
		bool isMapped = false;
		bool isAlive = false;

		SharedMemory() {
			memory = nullptr;
			device = nullptr;
			size = 0;
		}

		~SharedMemory()
		{
			if (memory && device)
			{
				vkFreeMemory(device, memory, nullptr);
			}
		}

		SharedMemory(const SharedMemory& other) = delete;

		VkResult map(VkDevice device, VkDeviceSize offset, VkMemoryMapFlags flags, VkDeviceSize mapSize = VK_WHOLE_SIZE) {
			if (isMapped) return VK_SUCCESS;
			const auto res = vkMapMemory(device, memory, offset, mapSize, flags, &mapped);
			if (res == VK_SUCCESS) isMapped = true;
			return res;
		}
		void unmap(VkDevice device) {
			if (!isMapped) return;
			vkUnmapMemory(device, memory);
			mapped = nullptr;
			isMapped = false;
		}
		void free(VkDevice device) {
			if (!isAlive) return;
			isAlive = false;
			vkFreeMemory(device, memory, nullptr);
		}
	};

	struct Buffer {
		VkDevice device = nullptr;
		VkBuffer buffer = nullptr;
		VkDescriptorBufferInfo descriptor{};
		uint32_t memoryOffset{};
		vkExt::SharedMemory* memory{};

		void* mapped() {
			assert(memory);
			if (!memory->isMapped) {
				const auto res = map();
				if (res != VK_SUCCESS) return nullptr;
			}
			return static_cast<void*>(static_cast<uint32_t *>(memory->mapped) + memoryOffset);
		}

		// Flags
		VkBufferUsageFlags usageFlags{};

		VkResult map(VkDeviceSize offset = 0) {
			if (!memory) return VK_ERROR_MEMORY_MAP_FAILED;
			const auto offs = offset == 0 ? descriptor.offset : offset;
			return memory->map(device, offs, 0);
		}

		void unmap() const {
			if (!memory) return;
			memory->unmap(device);
		}

		void bind(VkDeviceSize offset = 0) const {
			assert(memory);
			vkBindBufferMemory(device, buffer, memory->memory, offset);
		}

		void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) {
			descriptor.offset = offset;
			descriptor.buffer = buffer;
			descriptor.range = size;
		}

		void copyTo(const void* data, VkDeviceSize size, VkDeviceSize offset = 0) const {
			assert(memory->mapped);
			assert(offset + size <= memory->size);
			memcpy(static_cast<char*>(memory->mapped) + offset, data, static_cast<size_t>(size));
		}

		void copyToBuffer(VkCommandBuffer cmdBuffer, Buffer srcBuffer, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0) const {
			VkBufferCopy copyRegion = {
				descriptor.offset + srcOffset,
				dstOffset,
				size
			};
			vkCmdCopyBuffer(cmdBuffer, srcBuffer.buffer, buffer, 1, &copyRegion);
		}

		void copyToBuffer(VkCommandPool pool, VkQueue queue, Buffer srcBuffer, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0) const {
			const auto cmdBuff = beginSingleCommand(pool);
			copyToBuffer(cmdBuff, srcBuffer, size, srcOffset, dstOffset);
			endAndSubmitSingleCommand(cmdBuff, pool, queue);
		}

		void copyBufferToImage(VkCommandPool pool, VkQueue queue, VkImage image, uint32_t width, uint32_t height, VkDeviceSize srcOffset = 0) const {
			const auto cmdBuff = beginSingleCommand(pool);
				VkBufferImageCopy copyRegion = {
					descriptor.offset + srcOffset,
					0,
					0,
					{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
					{ 0, 0, 0 },
					{ width, height, 1 }
				};
				vkCmdCopyBufferToImage(cmdBuff, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
			endAndSubmitSingleCommand(cmdBuff, pool, queue);
		}

		void copyBufferToImage(VkCommandPool pool, VkQueue queue, VkImage image, uint32_t regionCount, VkBufferImageCopy* pRegions) {
			assert(pRegions != nullptr);
			assert(regionCount > 0);
			const auto cmdBuff = beginSingleCommand(pool);
				vkCmdCopyBufferToImage(cmdBuff, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regionCount, pRegions);
			endAndSubmitSingleCommand(cmdBuff, pool, queue);
		}

		VkResult flush() const {
			assert(memory);
			VkMappedMemoryRange mappedRange = {
				VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
				nullptr,
				memory->memory,
				descriptor.offset,
				descriptor.range
			};
			return vkFlushMappedMemoryRanges(device, 1, &mappedRange);
		}

		VkResult invalidate() const {
			assert(memory);
			VkMappedMemoryRange mappedRange = {
				VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
				nullptr,
				memory->memory,
				descriptor.offset,
				descriptor.range
			};
			return vkInvalidateMappedMemoryRanges(device, 1, &mappedRange);
		}

		void destroy(bool freeMem = false) const {
			unmap();
			if (freeMem && !memory) {
				std::cout << "Trying to free Memory for buffer (" << buffer<< ")" << std::endl;
			}
			if (buffer) {
				vkDestroyBuffer(device, buffer, nullptr);
			}
			if (freeMem) {
				if (!memory) {
					return;
				}
				memory->free(device);
			}
		}

	private:
		VkCommandBuffer beginSingleCommand(VkCommandPool pool) const {
			VkCommandBufferAllocateInfo allocInfo = {
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				nullptr,
				pool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1
			};

			VkCommandBuffer cmdBuff;
			vkAllocateCommandBuffers(device, &allocInfo, &cmdBuff);

			VkCommandBufferBeginInfo beginInfo = {
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				nullptr,
				VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
				nullptr
			};
			vkBeginCommandBuffer(cmdBuff, &beginInfo);
			return cmdBuff;
		}

		void endAndSubmitSingleCommand(VkCommandBuffer buffer, VkCommandPool pool, VkQueue queue) const {
			vkEndCommandBuffer(buffer);

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &buffer;

			vkQueueSubmit(queue, 1, &submitInfo, nullptr);
			const auto res = vkQueueWaitIdle(queue);
			if (res != VK_SUCCESS) {
				std::cerr << "QueueWait idle returned status " << res << std::endl;
			}

			vkFreeCommandBuffers(device, pool, 1, &buffer);
		}
	};

	struct Image {
		VkDevice device = nullptr;
		VkImage image = nullptr;
		vkExt::SharedMemory* memory{};
		VkDeviceSize offset = 0;
		uint32_t width{}, height{};
		VkFormat imageFormat{};

		void destroy(bool freeMem = false) const {
			vkDestroyImage(device, image, nullptr);
			if (freeMem) {
				if (!memory) return;
				memory->free(device);
			}
		}

		void copyTo(void* data, VkDeviceSize size) const {
			assert(memory->mapped);
			memcpy(memory->mapped, data, static_cast<size_t>(size));
		}

		VkResult map(VkDeviceSize offset = 0) {
			if (!memory) return VK_ERROR_MEMORY_MAP_FAILED;
			const auto offs = offset == 0 ? this->offset : offset;
			return memory->map(device, offs, 0);
		}

		void unmap() const {
			if (!memory) return;
			memory->unmap(device);
		}

		void bind(VkDeviceSize offset = 0) const {
			assert(memory);
			vkBindImageMemory(device, image, memory->memory, offset);
		}
	};
}

#endif // VULKAN_EXTENSION_H