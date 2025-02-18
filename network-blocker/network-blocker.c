// Following guide from: https://medium.com/@emanuele.santini.88/creating-a-linux-security-module-with-kprobes-blocking-network-of-targeted-processes-4046f50290f5

 #include <linux/module.h>
 #include <linux/kernel.h>
 #include <linux/kprobes.h>
 #include <linux/file.h>
 

 const char *exec_path = "/usr/bin/wget";
 
// hook functions
 const char *sendmsg_hook_name = "security_socket_sendmsg";
 const char *recvmsg_hook_name = "security_socket_recvmsg";
 const char *connect_hook_name = "security_socket_connect";
 const char *accept_hook_name = "security_socket_accept";



 static int probed_func_exit(struct kretprobe_instance *ri, struct pt_regs *regs);
 static int probed_func_entry(struct kretprobe_instance *ri, struct pt_regs *regs);
 

 #define declare_kretprobe(NAME, ENTRY_CALLBACK, EXIT_CALLBACK, DATA_SIZE) \
 static struct kretprobe NAME = {                                          \
     .handler	= EXIT_CALLBACK,	                                       \
     .entry_handler	= ENTRY_CALLBACK,				                       \
     .data_size	= DATA_SIZE,					                           \
     .maxactive	= NR_CPUS,					                               \
 };
 
 
 declare_kretprobe(sendmsg_probe, probed_func_entry, probed_func_exit, 0);
 declare_kretprobe(recvmsg_probe, probed_func_entry, probed_func_exit, 0);
 declare_kretprobe(connect_probe, probed_func_entry, probed_func_exit, 0);
 declare_kretprobe(accept_probe,  probed_func_entry, probed_func_exit, 0);
 

 
 static struct file* get_task_filep(struct task_struct *ctx)
 {
     struct file *exec_file_p = NULL;
     struct mm_struct *mm;
 
     if(unlikely(!ctx))
         return NULL;
 
     task_lock(ctx);
     mm = ctx->mm;
 
     if(mm && !(ctx->flags & PF_KTHREAD))
     {
         rcu_read_lock();
 
         exec_file_p = rcu_dereference(mm->exec_file_p);
         if(exec_file_p && !get_file_rcu(exec_file_p))
             exec_file_p = NULL;
 
         rcu_read_unlock();
     }
 
     task_unlock(ctx);
 
     return exec_file_p;
 }
 

 int probed_func_entry(struct kretprobe_instance *ri, struct pt_regs *regs)
 {
     struct file *fp;
     char *res;

     char f_path[256];
     memset(f_path, 0x0, 256);
 

     fp = get_task_filep(get_current());
     if(fp == NULL)
         return 1; // Do not call probed function exit
 
     if(IS_ERR(res = d_path(&fp->f_path, f_path, 256)))
         return 1;
 
    
     if(!strncmp(res, exec_path, 256))
     {
         printk("Blocking %s\n", res);
         return 0; // call probed function exit
     }
 
     fput(fp);
 
     return 1; // Do not call probed function exit
 }
 

 int probed_func_exit(struct kretprobe_instance *ri, struct pt_regs *regs)
 {
     regs->ax = -EACCES;
     return 0;
 }


 static int __init process_network_blocker_init(void)
 {
     sendmsg_probe.kp.symbol_name = sendmsg_hook_name;
     recvmsg_probe.kp.symbol_name = recvmsg_hook_name;
     connect_probe.kp.symbol_name = connect_hook_name;
     accept_probe.kp.symbol_name = accept_hook_name;
 
     register_kretprobe(&sendmsg_probe);
     register_kretprobe(&recvmsg_probe);
     register_kretprobe(&connect_probe);
     register_kretprobe(&accept_probe);
 
     return 0;
 }
 
 static void __exit process_network_blocker_exit(void)
 {
     unregister_kretprobe(&sendmsg_probe);
     unregister_kretprobe(&recvmsg_probe);
     unregister_kretprobe(&connect_probe);
     unregister_kretprobe(&accept_probe);
 }
 
 module_init(process_network_blocker_init);
 module_exit(process_network_blocker_exit);

 //meta
 MODULE_LICENSE("GPL v2");
 MODULE_AUTHOR("Adilet M");
 MODULE_DESCRIPTION("Network blocker for targeted process");