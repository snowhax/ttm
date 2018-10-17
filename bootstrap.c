#include "universal.h"

#define CPSR_T_MASK     ( 1u << 5 )

const char *libc_path = "/system/lib/libc.so";
const char *linker_path = "/system/bin/linker";
const char *libdl_path = "/system/lib/libdl.so";

const int long_size = sizeof(long);

void get_module_range(pid_t pid, const char* module_name, uint32_t* start_addr, uint32_t* end_addr) {
    FILE *fp;
    char *pch;
    char filename[32];
    char line[1024];
    *start_addr = 0;
    if (end_addr) {
        *end_addr = 0;
    }

    if (pid == 0) {
        snprintf(filename, sizeof(filename), "/proc/self/maps");
    } else {
        snprintf(filename, sizeof(filename), "/proc/%d/maps", pid);
    }

    fp = fopen(filename, "r");

    if (fp != NULL) {
        while (fgets(line, sizeof(line), fp)) {
            //LOGD("line: %s", line);
            if (strstr(line, module_name)) {
                pch = strtok(line, "-");
                *start_addr = strtoul(pch, NULL, 16);
                pch = strtok(NULL, "-");
                if (end_addr)
                    *end_addr = strtoul(pch, NULL, 16);
                break;
            }
        }
        fclose(fp) ;
    }
}

uint32_t get_remote_addr(pid_t target_pid, const char* module_name, void* local_addr)
{
    uint32_t local_handle, remote_handle;

    get_module_range(0, module_name, &local_handle, 0);
    get_module_range(target_pid, module_name, &remote_handle, 0);
    printf("remote %s handle = 0x%x\n", module_name, (unsigned int)remote_handle);

    long ret_addr = (long)((uint32_t)local_addr + (uint32_t)remote_handle - (uint32_t)local_handle);

    return ret_addr;
}

int ptrace_attach(pid_t pid)
{
    if (ptrace(PTRACE_ATTACH, pid, NULL, 0) < 0) {
        perror("ptrace_attach");
        return -1;
    }

    int status = 0;
    waitpid(pid, &status , WUNTRACED);

    return 0;
}

long ptrace_retval(struct pt_regs * regs)
{
#if defined(__arm__)
    return regs->ARM_r0;
#elif defined(__i386__)
    return regs->eax;
#else
#error "Not supported"
#endif
}

long ptrace_ip(struct pt_regs * regs)
{
#if defined(__arm__)
    return regs->ARM_pc;
#elif defined(__i386__)
    return regs->eip;
#else
#error "Not supported"
#endif
}

int ptrace_setregs(pid_t pid, struct pt_regs * regs)
{
    if (ptrace(PTRACE_SETREGS, pid, NULL, regs) < 0) {
        perror("ptrace_setregs: Can not set register values");
        return -1;
    }
    return 0;
}

int ptrace_getregs(pid_t pid, struct pt_regs * regs)
{
    if (ptrace(PTRACE_GETREGS, pid, NULL, regs) < 0) {
        perror("ptrace_getregs: Can not get register values");
        return -1;
    }
    return 0;
}

int ptrace_continue(pid_t pid)
{
    if (ptrace(PTRACE_CONT, pid, NULL, 0) < 0) {
        perror("ptrace_cont");
        return -1;
    }
    return 0;
}

int ptrace_detach(pid_t pid)
{
    if (ptrace(PTRACE_DETACH, pid, NULL, 0) < 0) {
        perror("ptrace_detach");
        return -1;
    }
    return 0;
}


int ptrace_readdata(pid_t pid,  uint8_t *src, uint8_t *buf, size_t size)
{
    uint32_t i, j, remain;
    uint8_t *laddr;

    union u {
        long val;
        char chars[sizeof(long)];
    } d;

    j = size / 4;
    remain = size % 4;

    laddr = buf;

    for (i = 0; i < j; i ++) {
        d.val = ptrace(PTRACE_PEEKTEXT, pid, src, 0);
        memcpy(laddr, d.chars, 4);
        src += 4;
        laddr += 4;
    }

    if (remain > 0) {
        d.val = ptrace(PTRACE_PEEKTEXT, pid, src, 0);
        memcpy(laddr, d.chars, remain);
    }

    return 0;
}

int ptrace_writedata(pid_t pid, uint8_t *dest, uint8_t *data, size_t size)
{
    uint32_t i, j, remain;
    uint8_t *laddr;

    union u {
        long val;
        char chars[sizeof(long)];
    } d;

    j = size / 4;
    remain = size % 4;

    laddr = data;

    for (i = 0; i < j; i ++) {
        memcpy(d.chars, laddr, 4);
        ptrace(PTRACE_POKETEXT, pid, dest, d.val);

        dest  += 4;
        laddr += 4;
    }

    if (remain > 0) {
        d.val = ptrace(PTRACE_PEEKTEXT, pid, dest, 0);
        for (i = 0; i < remain; i ++) {
            d.chars[i] = *laddr ++;
        }

        ptrace(PTRACE_POKETEXT, pid, dest, d.val);
    }

    return 0;
}

long ptrace_call(pid_t pid, uint32_t addr, uint32_t *params, uint32_t num_params, struct pt_regs * regs)
{
#if defined(__i386__)
    regs->eax = 0;
    regs->esp -= (num_params) * sizeof(uint32_t) ;
    ptrace_writedata(pid, (void *)regs->esp, (uint8_t *)params, (num_params) * sizeof(uint32_t));
    long tmp_addr = 0x00;
    regs->esp -= sizeof(long);
    ptrace_writedata(pid, (void *)regs->esp, (uint8_t *)&tmp_addr, sizeof(tmp_addr));
    regs->eip = addr;
#else
    uint32_t i;
    regs->uregs[0] = 0;
    //前面四个参数用寄存器传递
    for (i = 0; i < num_params && i < 4; i ++) {
      regs->uregs[i] = params[i];
    }
    //后面参数放到栈里
    if (i < num_params) {
      regs->ARM_sp -= (num_params - i) * sizeof(long) ;
      ptrace_writedata(pid, (void *)regs->ARM_sp, (uint8_t *)&params[i], (num_params - i) * sizeof(long));
    }
    //PC指向要执行的函数地址
    regs->ARM_pc = addr;
    if (regs->ARM_pc & 1) {
        /* thumb */
        regs->ARM_pc &= (~1u);
        regs->ARM_cpsr |= CPSR_T_MASK;
    } else {
        /* arm */
        regs->ARM_cpsr &= ~CPSR_T_MASK;
    }
  //把返回地址设为0，这样目标进程执行完返回时会出现地址错误，这样目标进程将被挂起，控制权会回到调试进程手中
  regs->ARM_lr = 0;

#endif

    if (ptrace_setregs(pid, regs) == -1 || ptrace_continue( pid) == -1) {
        printf("error\n");
        return -1;
    }

    int stat = 0;
    waitpid(pid, &stat, WUNTRACED);

    while (stat != 0xb7f) {
        if (ptrace_continue(pid) == -1) {
            printf("error\n");
            return -1;
        }
        waitpid(pid, &stat, WUNTRACED);
    }

    return 0;
}


int ptrace_call_wrapper(pid_t target_pid, const char * func_name, void * func_addr, uint32_t* parameters, int param_num, struct pt_regs * regs)
{
    LOGD("[+] Calling %s in target process.\n", func_name);
    if (ptrace_call(target_pid, (uint32_t)func_addr, parameters, param_num, regs) == -1)
        return -1;

    if (ptrace_getregs(target_pid, regs) == -1)
        return -1;
    LOGD("[+] Target process returned from %s, return value=%lx, pc=%lx \n",
            func_name, ptrace_retval(regs), ptrace_ip(regs));
    return 0;
}

int inject_remote_process(pid_t target_pid, const char *library_path, const char *function_name, const char *param, size_t param_size)
{
    int ret = -1;
    void *mmap_addr, *dlopen_addr, *dlsym_addr, *dlclose_addr, *dlerror_addr, *errno_addr;
    void *local_handle, *remote_handle, *dlhandle;
    uint32_t map_base = 0;
    int no = 0;
    int* pno = 0;
    const char* msg = 0;
    char pool[256] = {0};

    struct pt_regs regs, original_regs;

    uint32_t parameters[10];

    LOGD("[+] Injecting process: %d\n", target_pid);

    if (ptrace_attach(target_pid) == -1)
        goto exit;

    if (ptrace_getregs(target_pid, &regs) == -1)
        goto exit;

    /* save original registers */
    memcpy(&original_regs, &regs, sizeof(regs));

    mmap_addr = (void*)get_remote_addr(target_pid, libc_path, (void *)mmap);
    LOGD("[+] local mmap address: %x\n", (uint32_t)mmap);
    LOGD("[+] Remote mmap address: %x\n", (uint32_t)mmap_addr);

    /* call mmap */
    parameters[0] = 0;  // addr
    parameters[1] = 0x4000; // size
    //parameters[2] = PROT_READ | PROT_WRITE | PROT_EXEC;  // prot
    parameters[2] = PROT_READ | PROT_WRITE;
    parameters[3] =  MAP_ANONYMOUS | MAP_PRIVATE; // flags
    parameters[4] = 0; //fd
    parameters[5] = 0; //offset

    if (ptrace_call_wrapper(target_pid, "mmap", mmap_addr, parameters, 6, &regs) == -1)
        goto exit;

    map_base = ptrace_retval(&regs);
    LOGD("[+] map_base %p\n", (void *)map_base);

    if (map_base == -1) {
      errno_addr = (void*)get_remote_addr(target_pid, libc_path, (void *)__errno);
      LOGD("[+] local errno address: %x\n", (uint32_t)__errno);
      LOGD("[+] Remote errno address: %x\n", (uint32_t)errno_addr);
      if (ptrace_call_wrapper(target_pid, "errno", errno_addr, parameters, 0, &regs) == -1)
          goto exit;
      pno = ptrace_retval(&regs);
      ptrace_readdata(target_pid, pno, &no, 4);
      LOGD("[+] errno: %d\n", no);
    }

    dlopen_addr = (void*)get_remote_addr( target_pid, linker_path, (void *)dlopen );
    dlsym_addr = (void*)get_remote_addr( target_pid, linker_path, (void *)dlsym );
    dlclose_addr = (void*)get_remote_addr( target_pid, linker_path, (void *)dlclose );
    dlerror_addr = (void*)get_remote_addr( target_pid, linker_path, (void *)dlerror );

    LOGD("[+] Get imports: dlopen: %p, dlsym: %p, dlclose: %p, dlerror: %p\n",
            dlopen_addr, dlsym_addr, dlclose_addr, dlerror_addr);

    printf("library path = %s\n", library_path);
    ptrace_writedata(target_pid, (uint8_t *)map_base, (uint8_t *)library_path, strlen(library_path) + 1);

    parameters[0] = map_base;
    parameters[1] = RTLD_NOW| RTLD_GLOBAL;

    if (ptrace_call_wrapper(target_pid, "dlopen", dlopen_addr, parameters, 2, &regs) == -1)
        goto exit;

    uint32_t sohandle = (uint32_t)ptrace_retval(&regs);
    printf("dlopen:%08x\n", sohandle);

    if (sohandle == 0) {
      if (ptrace_call_wrapper(target_pid, "dlerror", dlerror_addr, parameters, 0, &regs) == -1)
          goto exit;
      msg = ptrace_retval(&regs);
      ptrace_readdata(target_pid, msg, &pool, 256);
      LOGD("[+] dlerror: %s\n", pool);
    }

#define FUNCTION_NAME_ADDR_OFFSET       0x100
    ptrace_writedata(target_pid, (uint8_t *)(map_base + FUNCTION_NAME_ADDR_OFFSET), (uint8_t *)function_name, strlen(function_name) + 1);
    parameters[0] = sohandle;
    parameters[1] = map_base + FUNCTION_NAME_ADDR_OFFSET;

    if (ptrace_call_wrapper(target_pid, "dlsym", dlsym_addr, parameters, 2, &regs) == -1)
        goto exit;

    void * hook_init_addr = (void *)ptrace_retval(&regs);
    LOGD("hook_init_addr = %p\n", hook_init_addr);

#define FUNCTION_PARAM_ADDR_OFFSET      0x200
    ptrace_writedata(target_pid, (uint8_t *)(map_base + FUNCTION_PARAM_ADDR_OFFSET), (uint8_t *)param, strlen(param) + 1);
    parameters[0] = map_base + FUNCTION_PARAM_ADDR_OFFSET;

    if (ptrace_call_wrapper(target_pid, "init_func", hook_init_addr, parameters, 1, &regs) == -1)
        goto exit;

#if 0
    printf("Press enter to dlclose and detach\n");
    getchar();
    parameters[0] = sohandle;

    if (ptrace_call_wrapper(target_pid, "dlclose", dlclose, parameters, 1, &regs) == -1)
        goto exit;
#endif

    /* restore */
	  //printf("detach\n");
    ptrace_setregs(target_pid, &original_regs);
    ptrace_detach(target_pid);
    ret = 0;

exit:
    return ret;
}

static int get_all_tids(pid_t pid, pid_t *tids)
{
    char dir_path[32];
    DIR *dir;
    int i;
    struct dirent *entry;
    pid_t tid;

    if (pid < 0) {
        snprintf(dir_path, sizeof(dir_path), "/proc/self/task");
    }
    else {
        snprintf(dir_path, sizeof(dir_path), "/proc/%d/task", pid);
    }

    dir = opendir(dir_path);
    if (dir == NULL) {
        return 0;
    }

    i = 0;
    while((entry = readdir(dir)) != NULL) {
        tid = atoi(entry->d_name);
        if (tid != 0 && tid != getpid()) {
            tids[i++] = tid;
        }
    }
    closedir(dir);
    return i;
}

int find_pid_of(const char* process_name)
{
	int id;
	pid_t pid = -1;
	DIR* dir;
	FILE *fp;
	char filename[128];
	char cmdline[256];

	struct dirent* entry;

	if(process_name == NULL)
	{
		return -1;
	}

	dir = opendir("/proc");

	if(dir ==NULL)
	{
		return -1;
	}
	while((entry=readdir(dir))!=NULL)
	{
		id = atoi(entry->d_name);
		if(id != 0)
		{
			sprintf(filename,"/proc/%d/cmdline",id);
			fp = fopen(filename,"r");
			if(fp)
			{
				fgets(cmdline, sizeof(cmdline), fp);
				fclose(fp);

				if(strcmp(process_name,cmdline)==0)
				{
					pid = id;
					break;
				}
			}
		}
	}
	closedir(dir);
	return pid;

}


int main(int argc, char *argv[])
{
    if (argc < 2) return -1;
    char* token = argv[1];
    pid_t pid = find_pid_of("zygote");
    if (!pid) return -1;

    char* so_path = "/data/local/tmp/libbridge.so";
    char* init_func = "init_func";
    char* parameter = token;
    inject_remote_process(pid, so_path, init_func, parameter, strlen(parameter));

    return 0;
}
