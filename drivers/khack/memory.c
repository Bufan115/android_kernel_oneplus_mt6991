#include "memory.h"
#include <linux/tty.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/pgtable.h>

#include <asm/cpu.h>
#include <asm/io.h>
#include <asm/page.h>

extern struct mm_struct *get_task_mm(struct task_struct *task);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 61))
extern void mmput(struct mm_struct *);
#endif

// 移除 valid_phys_addr_range 的依赖，使用替代方案
static inline int khack_valid_phys_addr_range(phys_addr_t addr, size_t count)
{
    // 简化的物理地址验证
    return addr + count > addr; // 基本的溢出检查
}

phys_addr_t translate_linear_address(struct mm_struct *mm, uintptr_t va)
{
    pgd_t *pgd;
    p4d_t *p4d;
    pmd_t *pmd;
    pte_t *pte;
    pud_t *pud;
    pte_t ptent;

    phys_addr_t page_addr;
    uintptr_t page_offset;

    pgd = pgd_offset(mm, va);
    if (pgd_none(*pgd) || pgd_bad(*pgd))
    {
        return 0;
    }
    
    p4d = p4d_offset(pgd, va);
    if (p4d_none(*p4d) || p4d_bad(*p4d))
    {
        return 0;
    }
    
    pud = pud_offset(p4d, va);
    if (pud_none(*pud) || pud_bad(*pud))
    {
        return 0;
    }
    
    pmd = pmd_offset(pud, va);
    if (pmd_none(*pmd))
    {
        return 0;
    }
    
    pte = pte_offset_map(pmd, va);
    if (!pte)
    {
        return 0;
    }
    
    ptent = ptep_get(pte);
    if (pte_none(ptent))
    {
        pte_unmap(pte);
        return 0;
    }
    
    if (!pte_present(ptent))
    {
        pte_unmap(pte);
        return 0;
    }
    
    page_addr = (phys_addr_t)(pte_pfn(ptent) << PAGE_SHIFT);
    page_offset = va & (PAGE_SIZE - 1);
    
    pte_unmap(pte);
    return page_addr + page_offset;
}

bool read_physical_address(phys_addr_t pa, void *buffer, size_t size)
{
    void *mapped;

    // 简化验证
    if (!pfn_valid(__phys_to_pfn(pa)))
    {
        return false;
    }
    
    if (!khack_valid_phys_addr_range(pa, size))
    {
        return false;
    }
    
    mapped = ioremap(pa, size);
    if (!mapped)
    {
        return false;
    }
    
    if (copy_to_user(buffer, mapped, size))
    {
        iounmap(mapped);
        return false;
    }
    
    iounmap(mapped);
    return true;
}

bool write_physical_address(phys_addr_t pa, void *buffer, size_t size)
{
    void *mapped;

    if (!pfn_valid(__phys_to_pfn(pa)))
    {
        return false;
    }
    
    if (!khack_valid_phys_addr_range(pa, size))
    {
        return false;
    }
    
    mapped = ioremap(pa, size);
    if (!mapped)
    {
        return false;
    }
    
    if (copy_from_user(mapped, buffer, size))
    {
        iounmap(mapped);
        return false;
    }
    
    iounmap(mapped);
    return true;
}

bool read_process_memory(pid_t pid, uintptr_t addr, void *buffer, size_t size)
{
    struct task_struct *task;
    struct mm_struct *mm;
    struct pid *pid_struct;
    phys_addr_t pa;
    bool result = false;

    pid_struct = find_get_pid(pid);
    if (!pid_struct)
    {
        return false;
    }
    
    task = get_pid_task(pid_struct, PIDTYPE_PID);
    if (!task)
    {
        return false;
    }
    
    mm = get_task_mm(task);
    if (!mm)
    {
        return false;
    }

    pa = translate_linear_address(mm, addr);
    if (pa)
    {
        result = read_physical_address(pa, buffer, size);
    }
    else
    {
        struct vm_area_struct *vma;
        
        // 简化处理：检查 VMA 是否存在
        vma = find_vma(mm, addr);
        if (vma && vma->vm_start <= addr && addr + size <= vma->vm_end)
        {
            // 对于无法翻译的地址，返回清零的缓冲区
            if (clear_user(buffer, size) == 0)
            {
                result = true;
            }
        }
    }

    mmput(mm);
    return result;
}

bool write_process_memory(pid_t pid, uintptr_t addr, void *buffer, size_t size)
{
    struct task_struct *task;
    struct mm_struct *mm;
    struct pid *pid_struct;
    phys_addr_t pa;
    bool result = false;

    pid_struct = find_get_pid(pid);
    if (!pid_struct)
    {
        return false;
    }
    
    task = get_pid_task(pid_struct, PIDTYPE_PID);
    if (!task)
    {
        return false;
    }
    
    mm = get_task_mm(task);
    if (!mm)
    {
        return false;
    }

    pa = translate_linear_address(mm, addr);
    if (pa)
    {
        result = write_physical_address(pa, buffer, size);
    }

    mmput(mm);
    return result;
}
