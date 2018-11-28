#include <system/gpu_utils.h>

void *gpu_alloc(SceKernelMemBlockType type, unsigned int size, unsigned int alignment, SceGxmMemoryAttribFlags attribs, SceUID *uid) {
	void *mem;

	if (type == SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW) {
		size = ALIGN(size, 256*1024);
	} else {
		size = ALIGN(size, 4*1024);
	}

	*uid = sceKernelAllocMemBlock("gpu_mem", type, size, NULL);

	if (*uid < 0)
		return NULL;

	if (sceKernelGetMemBlockBase(*uid, &mem) < 0)
		return NULL;

	if (sceGxmMapMemory(mem, size, attribs) < 0)
		return NULL;

	return mem;
}

void gpu_free(SceUID uid) {
	void *mem = NULL;
	if (sceKernelGetMemBlockBase(uid, &mem) < 0)
		return;
	sceGxmUnmapMemory(mem);
	sceKernelFreeMemBlock(uid);
}

void *vertex_usse_alloc(unsigned int size, SceUID *uid, unsigned int *usse_offset) {
	void *mem = NULL;

	size = ALIGN(size, 4096);
	*uid = sceKernelAllocMemBlock("vertex_usse", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);

	if (sceKernelGetMemBlockBase(*uid, &mem) < 0)
		return NULL;
	if (sceGxmMapVertexUsseMemory(mem, size, usse_offset) < 0)
		return NULL;

	return mem;
}

void vertex_usse_free(SceUID uid) {
	void *mem = NULL;
	if (sceKernelGetMemBlockBase(uid, &mem) < 0)
		return;
	sceGxmUnmapVertexUsseMemory(mem);
	sceKernelFreeMemBlock(uid);
}

void *fragment_usse_alloc(unsigned int size, SceUID *uid, unsigned int *usse_offset) {
	void *mem = NULL;

	size = ALIGN(size, 4096);
	*uid = sceKernelAllocMemBlock("fragment_usse", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, size, NULL);

	if (sceKernelGetMemBlockBase(*uid, &mem) < 0)
		return NULL;
	if (sceGxmMapFragmentUsseMemory(mem, size, usse_offset) < 0)
		return NULL;

	return mem;
}

void fragment_usse_free(SceUID uid) {
	void *mem = NULL;
	if (sceKernelGetMemBlockBase(uid, &mem) < 0)
		return;
	sceGxmUnmapFragmentUsseMemory(mem);
	sceKernelFreeMemBlock(uid);
}
