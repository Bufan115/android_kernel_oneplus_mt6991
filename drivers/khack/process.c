#include <linux/sched.h>
#include <linux/sched/mm.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/dcache.h>
#include "process.h"

uintptr_t get_module_base(pid_t pid, const char *name)
{
    struct task_struct *task;
    struct mm_struct *mm;
    struct vm_area_struct *vma;
    uintptr_t base = 0;
    
    task = get_pid_task(find_get_pid(pid), PIDTYPE_PID);
    if (!task)
        return 0;
    
    mm = get_task_mm(task);
    if (!mm)
        goto out_task;
    
    // 使用find_vma进行兼容性遍历
    unsigned long addr = 0;
    while ((vma = find_vma(mm, addr)) != NULL) {
        if (vma->vm_file) {
            const char *vma_name = vma->vm_file->f_path.dentry->d_name.name;
            if (vma_name && strcmp(vma_name, name) == 0) {
                base = vma->vm_start;
                break;
            }
        }
        
        // 移动到下一个VMA
        if (vma->vm_end > addr)
            addr = vma->vm_end;
        else
            break;
            
        // 防止无限循环
        if (addr == 0)
            break;
    }
    
    mmput(mm);
out_task:
    put_task_struct(task);
    return base;
}
