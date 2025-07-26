void _start() {
    __asm__ volatile("mov $0x90000, %esp");

    char* vid_mem = (char*)0xB8000;
    
    for (int i=0; i < 2000; i+= 2){
        vid_mem[i] = ' ';
        vid_mem[i+1] = 0x0F;
    }

    const char* msg = "Kernel is active";
    
    int i = 0;
    while (msg[i] != '\0')
    {
        vid_mem[i*2]=msg[i];
        vid_mem[i*2+1]=0x0F;
        i++;
    }
    
    
    while(1){
        __asm__ volatile("hlt");
    }

}