#include "process.h"
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/mm.h>
#include <linux/version.h>
#if(LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0))
#include <linux/sched/mm.h>
#endif
#define ARC_PATH_MAX 256

    struct vm_area_struct *vma;
VMA_ITERATOR(iter, mm, 0);

extern struct mm_struct *get_task_mm(struct task_struct *task);
/*
#if(LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 61))
extern void mmput(struct mm_struct *);
#endif
*/

static unsigned long get_module_base(pid_t pid, const char *name)
{
    struct task_struct *task;
    struct mm_struct *mm;
    struct vm_area_struct *vma;
    unsigned long base = 0;
    
    // 获取任务结构
    task = get_pid_task(find_get_pid(pid), PIDTYPE_PID);
    if (!task)
        return 0;
    
    // 获取内存管理结构
    mm = get_task_mm(task);
    if (!mm)
        goto out_task;
    
    // 使用兼容的VMA遍历方法
    // 方法1: 尝试使用现代内核的VMA迭代器
#ifdef CONFIG_MMU
    {
        VMA_ITERATOR(iter, mm, 0);
        for_each_vma(iter, vma) {
            if (vma->vm_file) {
                const char *vma_name = vma->vm_file->f_path.dentry->d_name.name;
                if (strcmp(vma_name, name) == 0) {
                    base = vma->vm_start;
                    break;
                }
            }
        }
    }
#else
    // 方法2: 传统VMA遍历
    for (vma = mm->mmap; vma; vma = vma->vm_next) {
        if (vma->vm_file) {
            const char *vma_name = vma->vm_file->f_path.dentry->d_name.name;
            if (strcmp(vma_name, name) == 0) {
                base = vma->vm_start;
                break;
            }
        }
    }
#endif
    
    mmput(mm);
out_task:
    put_task_struct(task);
    return base;
}
