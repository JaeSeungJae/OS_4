#include <stdio.h>
#include <stdint.h> 
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <string.h>
#include <time.h>

uint8_t* Operation;
uint8_t* compiled_code;

void sharedmem_init(); // ���� �޸𸮿� ����
void sharedmem_exit();
void drecompile_init(); // memory mapping ���� 
void drecompile_exit(); 
void* drecompile(uint8_t *func); //����ȭ�ϴ� �κ�
uint8_t* buffer;
int segment_id;
int size = PAGE_SIZE;

int main(void)
{
	int (*func)(int a);
	int i;
	clock_t start, end;
	double exec_time;
	
	sharedmem_init();
	drecompile_init();
	func = (int (*)(int))drecompile(Operation);
	start = clock();
	int result;
	for (int i = 0; i < 1000; i++) {
		result = func(1);
	}
	end = clock();
	//printf("%d\n", result);
	//printf("result : %d\n", func(1));
	exec_time = ((double) (end - start)) / CLOCKS_PER_SEC;
	printf("total execution time : %f sec\n", exec_time);
	drecompile_exit();
	sharedmem_exit();
	return 0;
}

void sharedmem_init()
{
	segment_id = shmget(1234, size, IPC_CREAT | S_IRUSR | S_IWUSR);
	Operation = (uint8_t*)shmat(segment_id, NULL, 0); // shared memory == address
}

void sharedmem_exit()
{
	shmdt(Operation);
	shmctl(segment_id, IPC_RMID, NULL);
}

void drecompile_init() // copy function "Operation"
{
	buffer = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	memcpy(buffer, Operation, size);
}

void drecompile_exit()
{
	if (mprotect(buffer, size, PROT_READ | PROT_EXEC) == -1) {
        munmap(buffer, size);
    }
	if (mprotect(compiled_code, size, PROT_READ | PROT_EXEC) == -1) {
        munmap(compiled_code, size);
    }
}

void* drecompile(uint8_t* func)
{
	#ifdef dynamic
	compiled_code = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	int i = 0;
	uint8_t add = 0;
	uint8_t sub = 0;
	uint8_t mul = 1;
	uint8_t div = 1;
	uint8_t divc = 1;
	while (*buffer != 0xC3) {
		if (*buffer == 0x83 && *(buffer + 1) == 0xC0) {
			add += *(buffer + 2);
			if (sub != 0) {
				compiled_code[i] = 0x83;
				compiled_code[i+1] = 0xE8;
				compiled_code[i+2] = sub;
				sub = 0;
				i += 3;
			}
			if (mul != 1) {
				compiled_code[i] = 0x6B;
				compiled_code[i+1] = 0xC0;
				compiled_code[i+2] = mul;
				mul = 1;
				i += 3;
			}
			if (div != 1) {
				compiled_code[i] = 0xB2;
				compiled_code[i+1] = div;
				compiled_code[i+2] = 0xF6;
				compiled_code[i+3] = 0xF2;
				div = 1;
				i += 4;
			}
			buffer += 3;
		}
		else if (*buffer == 0x83 && *(buffer + 1) == 0xE8) {
			sub += *(buffer + 2);
			if (add != 0) {
				compiled_code[i] = 0x83;
				compiled_code[i+1] = 0xC0;
				compiled_code[i+2] = add;
				add = 0;
				i += 3;
			}
			if (mul != 1) {
				compiled_code[i] = 0x6B;
				compiled_code[i+1] = 0xC0;
				compiled_code[i+2] = mul;
				mul = 1;
				i += 3;
			}
			if (div != 1) {
				compiled_code[i] = 0xB2;
				compiled_code[i+1] = div;
				compiled_code[i+2] = 0xF6;
				compiled_code[i+3] = 0xF2;
				div = 1;
				i += 4;
			}
			buffer += 3;
		}
		else if (*buffer == 0x6B && *(buffer + 1) == 0xC0) {
			mul *= *(buffer + 2);
			if (sub != 0) {
				compiled_code[i] = 0x83;
				compiled_code[i+1] = 0xE8;
				compiled_code[i+2] = sub;
				sub = 0;
				i += 3;
			}
			if (add != 0) {
				compiled_code[i] = 0x83;
				compiled_code[i+1] = 0xC0;
				compiled_code[i+2] = add;
				add = 0;
				i += 3;
			}
			if (div != 1) {
				compiled_code[i] = 0xB2;
				compiled_code[i+1] = div;
				compiled_code[i+2] = 0xF6;
				compiled_code[i+3] = 0xF2;
				div = 1;
				i += 4;
			}
			buffer += 3;
		}
		else if (*buffer == 0xF6 && *(buffer + 1) == 0xF2) {
			div *= divc;
			if (sub != 0) {
				compiled_code[i] = 0x83;
				compiled_code[i+1] = 0xE8;
				compiled_code[i+2] = sub;
				sub = 0;
				i += 3;
			}
			if (add != 0) {
				compiled_code[i] = 0x83;
				compiled_code[i+1] = 0xC0;
				compiled_code[i+2] = add;
				add = 0;
				i += 3;
			}
			if (mul != 1) {
				compiled_code[i] = 0x6B;
				compiled_code[i+1] = 0xC0;
				compiled_code[i+2] = mul;
				mul = 1;
				i += 3;
			}
			buffer += 2;
		}
		else if (*buffer == 0xB2) {
			compiled_code[i] = *buffer;
			compiled_code[i+1] = *(buffer + 1);
			divc = *(buffer + 1);
			i += 2;
			buffer += 2;
		}
		else {
			if (sub != 0) {
				compiled_code[i] = 0x83;
				compiled_code[i+1] = 0xE8;
				compiled_code[i+2] = sub;
				sub = 0;
				i += 3;
			}
			if (add != 0) {
				compiled_code[i] = 0x83;
				compiled_code[i+1] = 0xC0;
				compiled_code[i+2] = add;
				add = 0;
				i += 3;
			}
			if (mul != 1) {
				compiled_code[i] = 0x6B;
				compiled_code[i+1] = 0xC0;
				compiled_code[i+2] = mul;
				mul = 1;
				i += 3;
			}
			if (div != 1) {
				compiled_code[i] = 0xB2;
				compiled_code[i+1] = div;
				compiled_code[i+2] = 0xF6;
				compiled_code[i+3] = 0xF2;
				div = 1;
				i += 4;
			}
			compiled_code[i++] = *buffer;
			buffer++;
		}
	}
	compiled_code[i] = 0xC3; 
	
	return compiled_code;
	#endif

	return buffer;
}

