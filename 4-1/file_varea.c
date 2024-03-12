#include <linux/module.h>
#include <linux/highmem.h>
#include <linux/kallsyms.h>
#include <linux/syscalls.h>
#include <asm/syscall_wrapper.h>
#include <linux/sched.h>
#include <linux/init_task.h>
#include <linux/init.h>
#include <linux/pid.h>
#include <linux/kernel.h>
#include <linux/ftrace.h>
#include <linux/unistd.h>

#define __NR_ftrace 336

void **syscall_table;
void *real_ftrace;

asmlinkage pid_t file_varea(const struct pt_regs *regs) {
    pid_t trace_task = (pid_t)regs->di;
    struct task_struct *task;
    task = pid_task(find_vpid(trace_task), PIDTYPE_PID);
    if (!task) {
        return -1;
    }
    struct mm_struct *mm = task->mm;
    struct vm_area_struct *vma = mm->mmap;
    printk("####### Loaded files of a process '%s(%d)' in VM #######\n", task->comm, trace_task);
    while (vma) {
        char* buf;
        char* path = NULL;
        if (vma->vm_file) { // vma가 파일에 매핑된 경우
            buf = (char *)__get_free_page(GFP_KERNEL);
            if (buf) {
                path = d_path(&vma->vm_file->f_path, buf, PAGE_SIZE);
                if (IS_ERR(path)) {
                    path = NULL;
                }
            }
        }
        if (path != NULL){
        printk("mem[%lx~%lx] code[%lx~%lx] data[%lx~%lx] heap[%lx~%lx] %s\n", vma->vm_start, vma->vm_end, vma->vm_mm->start_code, vma->vm_mm->end_code, 
        vma->vm_mm->start_data, vma->vm_mm->end_data, vma->vm_mm->start_brk, vma->vm_mm->brk, path);
        }
        vma = vma->vm_next;
    }
    printk("##########################################################\n");
    
}
void make_ro(void *addr) {
    unsigned int level;
    pte_t *pte = lookup_address((u64)addr, &level);
    pte->pte = pte->pte &~ _PAGE_RW;
}
void make_rw(void *addr) {
    unsigned int level;
    pte_t *pte = lookup_address((u64)addr, &level);
    if (pte->pte &~ _PAGE_RW)
        pte->pte |= _PAGE_RW;
}
static int __init tracer_init(void) {
    syscall_table = (void**) kallsyms_lookup_name("sys_call_table");
    make_rw(syscall_table);
    real_ftrace = syscall_table[__NR_ftrace];
    syscall_table[__NR_ftrace] = file_varea;
    return 0;
}
static void __exit tracer_exit(void) {
    syscall_table[__NR_ftrace] = real_ftrace;
    make_ro(syscall_table);
}
module_init(tracer_init);
module_exit(tracer_exit);
MODULE_LICENSE("GPL");