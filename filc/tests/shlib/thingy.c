int thingy(void)
{
    return 666;
}

extern __typeof(thingy) stuff __attribute__((__weak__, __alias__("thingy")));

