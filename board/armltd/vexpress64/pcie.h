#ifndef __VEXPRESS64_PCIE_H__
#define __VEXPRESS64_PCIE_H__

#ifdef CONFIG_TARGET_VEXPRESS64_JUNO
void vexpress64_pcie_init(void);
#else
static inline void vexpress64_pcie_init(void) {}
#endif

#endif /* __VEXPRESS64_PCIE_H__ */
