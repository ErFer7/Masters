#include <utility/ostream.h>
#include <utility/elf.h>
#include <architecture.h>
#include <memory.h>
#include <process.h>

using namespace EPOS;

OStream cout;

typedef int (Main)(int argc, char * argv[]);

int main(int argc, char * argv[])
{
    cout << "EPOS application loader:\n" << endl;

    Task * self = Task::self();

    if(!argc) {
        cout << "Got no applications to load. Exiting!" << endl;
        return -1;
    }

    cout << "Got " << argc << " bytes of applications to load from " << reinterpret_cast<void *>(argv) << "\n" << endl;

    char * extra = reinterpret_cast<char *>(argv);
    long extra_size = argc;
    long size = *reinterpret_cast<int *>(extra);
    while((extra_size > 0) && size) {
        extra += sizeof(long);
        extra_size = -sizeof(long);

        cout << "Loading application of size " << size << " from " << reinterpret_cast<void *>(extra) << " ... " << endl;

        ELF * elf = reinterpret_cast<ELF *>(extra);

        if(elf->valid()) {
            unsigned long entry = elf->entry();
            cout << "  entry=" << hex << entry << ", segments=" << dec << elf->segments() << endl;
            unsigned long code_addr = elf->segment_address(0);
            unsigned long code_size = elf->segment_size(0);
            cout << "  code =" << hex << code_addr << ", size=" << dec << code_size << endl;
            unsigned long data_addr = ~0U;
            unsigned long data_size = 0;
            if(elf->segments() > 1) {
                for(unsigned int i = 1; i < elf->segments(); i++) {
                    if(elf->segment_type(i) != PT_LOAD)
                        continue;
                    if(elf->segment_address(i) < data_addr)
                        data_addr = elf->segment_address(i);

                    data_size += elf->segment_size(i);
                }
            }
            if(data_addr == ~0U)
                cout << "  image has no data segment" << endl;
            data_size += EPOS::S::Traits<Application>::STACK_SIZE;
            data_size += EPOS::S::Traits<Application>::HEAP_SIZE;
            cout << "  data =" << hex << data_addr << ", size=" << dec << data_size << endl;

            Segment * cs = new Segment(code_size);
            Segment::Log_Addr code = self->address_space()->attach(cs);
            elf->load_segment(0, code);
            self->address_space()->detach(cs, code);

            Segment * ds = new Segment(data_size);
            Segment::Log_Addr data = self->address_space()->attach(ds);
            for(unsigned int i = 1; i < elf->segments(); i++) {
                if(elf->segment_size(i) > 0)
                    elf->load_segment(i, data);
                data += elf->segment_size(i);
            }
            self->address_space()->detach(ds);

            new Task(cs, ds, code_addr, data_addr, reinterpret_cast<Main *>(entry), 0, reinterpret_cast<char **>(0));

            cout << "done!" << endl;
        } else
            cout << "error: the ELF image is corrupted!" << endl;

        extra += size;
        extra_size -= size;
        size = *reinterpret_cast<int *>(extra);
    }

    cout << "Finish!" << endl;

    return 0;
}
