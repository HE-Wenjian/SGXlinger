#include <stdio.h>
#include "../kernel_module/sgxlinger.h"

// const char *pos_path = "/sys/kernel/debug/sgxlinger/data_pos";
const char *data_path = "/sys/kernel/debug/sgxlinger/monitor_data";

int main()
{
    FILE *ifp;
    ifp = fopen(data_path, "rb");
    if (!ifp)
    {
        fprintf(stderr, "[ERR] open file failed, ensure to use sudo to launch this program.\n");
        return 1;
    }

    size_t rd = 1;
    size_t nullcnt = 0;
    struct SGXlingerDataEntry buf[4096];
    while (rd > 0)
    {
        rd = fread(buf, sizeof(struct SGXlingerDataEntry), 4096, ifp);
        for (int i = 0; i < rd; i++)
        {
            if (buf[i].interrupt_timestamp == 0)
            {
                nullcnt++;
                if (nullcnt > 5)
                {
                    break;
                }
            }
            else
            {
                nullcnt = 0;
                printf("%llu > %u\n",
                       buf[i].interrupt_timestamp,
                       buf[i].interrupt_delay);
            }
        }
    }

    fclose(ifp);
    return 0;
}