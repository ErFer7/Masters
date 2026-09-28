#ifdef __NIC_H

#include <machine/riscv/visionfive2/visionfive2_gmac.h>  // TODO: Change to #include <machine/nic.h> and check it


__BEGIN_SYS

DWC_Ether_QoS *DWC_Ether_QoS::s_devices[DWC_Ether_QoS::k_number_of_descriptors];

int DWC_Ether_QoS::send(Buffer *buf) {
    db<DWC_Ether_QoS>(TRC) << "DWC_Ether_QoS::send(buf=" << buf << ")" << endl;

    m_dma->send(buf->frame(), buf->size());

    return buf->size();
}

int DWC_Ether_QoS::send(const Address &dst, const Protocol &prot, const void *data, unsigned int size) {
    db<DWC_Ether_QoS>(TRC) << "DWC_Ether_QoS::send(s=" << m_configuration.address << ",d=" << dst << ",p=" << hex
                           << prot << dec << ",d=" << data << ",s=" << size << ")" << endl;

    int i = 0;
    for (bool locked = false; !locked;) {
        i = (i + 1) % k_number_of_descriptors;
        locked = m_tx_buffers[i]->lock();
    }

    Buffer *buffer = m_tx_buffers[i];
    new (buffer->frame()) Frame(m_configuration.address, dst, prot, data, size);

    m_dma->send(buffer->frame(), size + sizeof(Header));

    return size;
}

int DWC_Ether_QoS::receive(Address *src, Protocol *prot, void *data, unsigned int size) {
    db<DWC_Ether_QoS>(TRC) << "DWC_Ether_QoS::receive(s=" << *src << ",p=" << hex << *prot << dec << ",d=" << data
                           << ",s=" << size << ") => " << endl;

    Buffer buffer(this, nullptr);
    m_dma->receive(buffer.data(), buffer.size());

    Frame *frame = buffer.frame();
    *src = frame->src();
    *prot = frame->prot();

    memcpy(data, frame->data<void>(), (buffer.size() > size) ? size : buffer.size());

    return buffer.size();
}

DWC_Ether_QoS::Buffer *DWC_Ether_QoS::alloc(const Address &dst, const Protocol &prot, unsigned int once,
                                            unsigned int always, unsigned int payload) {
    db<DWC_Ether_QoS>(TRC) << "DWC_Ether_QoS::alloc(s=" << m_configuration.address << ",d=" << dst << ",p=" << hex
                           << prot << dec << ",on=" << once << ",al=" << always << ",pl=" << payload << ")" << endl;
    int max_data = MTU - always;

    if((payload + once) / max_data > k_number_of_descriptors) {
        db<DWC_Ether_QoS>(WRN) << "DWC_Ether_QoS::alloc: sizeof(Network::Packet::Data) > sizeof(NIC::Frame::Data) * k_number_of_descriptors!" << endl;
        return 0;
    }

    Buffer::List pool;

    for(int size = once + payload; size > 0; size -= max_data) {
        unsigned int i = 0;
        for (bool locked = false; !locked;) {
            i = (i + 1) % k_number_of_descriptors;
            locked = m_tx_buffers[i]->lock();
        }

        Buffer * buf = m_tx_buffers[i];

        int actual_size = (size > max_data) ? MTU : size + always;

        buf->fill(actual_size, m_configuration.address, dst, prot);

        db<DWC_Ether_QoS>(INF) << "DWC_Ether_QoS::alloc:buf[" << i << "]=" << buf << " => " << *buf << endl;

        pool.insert(buf->link());
    }

    return pool.head()->object();
}

void DWC_Ether_QoS::free(Buffer *buf) { db<DWC_Ether_QoS>(TRC) << "DWC_Ether_QoS::free(buf=" << buf << ")" << endl; }

__END_SYS

#endif
