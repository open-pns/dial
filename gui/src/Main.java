import javax.swing.SwingUtilities;

public class Main {
    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            DialFrame frame = new DialFrame();
            frame.setVisible(true);
            new SerialReader(frame).start();
        });
    }
}
