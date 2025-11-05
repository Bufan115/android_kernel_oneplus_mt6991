#include "process.h"
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/tty.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/fs.h>

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
    struct vm_area_struct *vma;
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

    // 使用兼容的 VMA 遍历方式
    for (vma = mm->mmap; vma; vma = vma->vm_next)
    {
        char *path_nm = NULL;

        if (vma->vm_file)
        {
            char *dentry_path = d_path(&vma->vm_file->f_path, (char *)__get_free_page(GFP_KERNEL), PAGE_SIZE);
            if (!IS_ERR(dentry_path))
            {
                path_nm = kbasename(dentry_path);
                if (path_nm && !strcmp(path_nm, name))
                {
                    base_addr = vma->vm_start;
                    free_page((unsigned long)dentry_path);
                    break;
                }
                free_page((unsigned long)dentry_path);
            }
        }
    }

    mmput(mm);
    return base_addr;
}
