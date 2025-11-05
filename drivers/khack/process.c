#include "process.h"
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/dcache.h>
#include <linux/mm_types.h>

#define ARC_PATH_MAX 256

extern struct mm_struct *get_task_mm(struct task_struct *task);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 61))
extern void mmput(struct mm_struct *);
#endif

uintptr_t get_module_base(pid_t pid, char *name)
{
    struct pid *pid_struct;
    struct task_struct *task;
    struct mm_struct *mm;
    uintptr_t base_addr = 0;

    pid_struct = find_get_pid(pid);
    if (!pid_struct)
    {
        return 0;
    }
    
    task = get_pid_task(pid_struct, PIDTYPE_PID);
    if (!task)
    {
        return 0;
    }
    
    mm = get_task_mm(task);
    if (!mm)
    {
        return 0;
    }

    // 使用现代内核的 VMA 遍历方式
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
    // 使用 VMA 迭代器（新内核）
    struct vm_area_struct *vma;
    VMA_ITERATOR(vmi, mm, 0);
    for_each_vma(vmi, vma)
    {
        char *path_nm = NULL;
        char *dentry_path;

        if (vma->vm_file)
        {
            dentry_path = (char *)__get_free_page(GFP_KERNEL);
            if (dentry_path)
            {
                char *p = file_path(vma->vm_file, dentry_path, PAGE_SIZE);
                if (!IS_ERR(p))
                {
                    path_nm = kbasename(p);
                    if (path_nm && !strcmp(path_nm, name))
                    {
                        base_addr = vma->vm_start;
                        free_page((unsigned long)dentry_path);
                        break;
                    }
                }
                free_page((unsigned long)dentry_path);
            }
        }
    }
#else
    // 对于旧内核的兼容处理
    // 如果上述方法不行，我们使用更简单的方法：返回 0 表示不支持
    base_addr = 0;
#endif

    mmput(mm);
    return base_addr;
}
