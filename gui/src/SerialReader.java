//heavily relying on this for port reading
import com.fazecast.jSerialComm.SerialPort;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.awt.Color;

//we want serialReader to run on a seperate thread
public class SerialReader extends Thread {

    private static final int BAUD = 115200;

    private final DialFrame frame;

    public SerialReader(DialFrame frame) {
        this.frame = frame; //bring frame to write to in scope
        setDaemon(true); //thread needs this
    }

    @Override
    public void run() {
        while (true) {
            SerialPort port = findPort();
            if (port == null) {
                frame.setStatus("No device found. retrying...", Color.RED);
                sleep(2000);
                continue;
            }

            port.setBaudRate(BAUD);
            port.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING, 0, 0);

            if (!port.openPort()) {
                frame.setStatus("Could not open port. retrying...", Color.RED);
                sleep(2000);
                continue;
            }

            frame.setStatus("Connected: " + port.getSystemPortName(), new Color(0, 140, 0));
            sleep(500);

            try (BufferedReader reader = new BufferedReader(
                    new InputStreamReader(port.getInputStream()))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    String[] p = line.split(",");
                    if (p.length != 6) continue;
                    float rotX = Float.parseFloat(p[0]);
                    float rotY = Float.parseFloat(p[1]);
                    float rotZ = Float.parseFloat(p[2]);
                    float adc  = Float.parseFloat(p[3]);
                    float thr  = Float.parseFloat(p[4]);
                    int   dial = Integer.parseInt(p[5].trim());
                    frame.updateData(rotX, rotY, rotZ, adc, thr, dial);
                }
            } catch (Exception e) {
                System.out.println("Read exception: " + e.getMessage());
            } finally {
                port.closePort();
            }

            frame.setStatus("Disconnected. reconnecting...", Color.RED);
            sleep(1000);
        }
    }

    //function for troubleshooting. Sometimes the board disconnects itself.
    private SerialPort findPort() {
        //iterate through serialPorts to find the one connected over usb (the rp2350 proton board)
        for (SerialPort p : SerialPort.getCommPorts()) {
            String name = p.getSystemPortName();
            //return usb serial port
            if (name.contains("usbmodem") || name.contains("usbserial")) return p;
        }
        return null;
    }

    //basic sleep function
    private void sleep(int ms) {
        try { Thread.sleep(ms); } catch (InterruptedException ignored) {}
    }
}
