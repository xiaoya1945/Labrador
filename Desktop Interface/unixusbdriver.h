#ifndef unixUsbDriver_H
#define unixUsbDriver_H

#include <QWidget>
#include <QThread>
#include <QMutex>
#include <QDateTime>

#include "genericusbdriver.h"
#include "libusb.h"

#define RECOVERY_PERIOD 250

//tcBlock is fed to the callback in the libusb user data section.
typedef struct tcBlock{
    int number;
    bool completed;
    qint64 timeReceived;
} tcBlock;

extern QMutex tcBlockMutex;

//Oddly, libusb requires you to make a blocking libusb_handle_events() call in order to execute the callbacks for an asynchronous transfer.
//Since the call is blocking, this worker must exist in a separate, low priority thread!
class worker : public QObject
{
    Q_OBJECT

public:
    worker(){};
    ~worker(){};
    libusb_context *ctx;
public slots:
    void handle(){
        qDebug() << "SUB THREAD ID" << QThread::currentThreadId();
        while(1){
            if(libusb_event_handling_ok(ctx)){
                libusb_handle_events(ctx);
                //qDebug() << "HANDLED";
            }
        }
    }
};

//This is the actual unixUsbDriver
//It handles the Mac/Linux specific parts of USB communication, through libusb.
//See genericUsbDriver for the non-platform-specific parts.
class unixUsbDriver : public genericUsbDriver
{
    Q_OBJECT
public:
    explicit unixUsbDriver(QWidget *parent = 0);
    ~unixUsbDriver();
    void usbSendControl(uint8_t RequestType, uint8_t Request, uint16_t Value, uint16_t Index, uint16_t Length, unsigned char *LDATA);
    char *isoRead(unsigned int *newLength);
private:
    //USB Vars
    libusb_context *ctx;
    libusb_device_handle *handle = NULL;
    QTimer *recoveryTimer;
    //USBIso Vars
    libusb_transfer *isoCtx[NUM_FUTURE_CTX];
    tcBlock transferCompleted[NUM_FUTURE_CTX];
    unsigned char dataBuffer[NUM_FUTURE_CTX][ISO_PACKET_SIZE*ISO_PACKETS_PER_CTX];
    worker *isoHandler;
    QThread *workerThread;
    //Generic Functions
    unsigned char usbInit(unsigned long VIDin, unsigned long PIDin);
    unsigned char usbIsoInit(void);
signals:
public slots:
    void isoTimerTick(void);
    void recoveryTick(void);
};

//Callback on iso transfer complete.
static void LIBUSB_CALL isoCallback(struct libusb_transfer *transfer);

#endif // unixUsbDriver_H
