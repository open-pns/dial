import javax.swing.*;
import java.awt.*;

public class DialFrame extends JFrame {

    private final JLabel       rotXVal;
    private final JLabel       rotYVal;
    private final JLabel       rotZVal;
    private final JLabel       adcVal;
    private final JLabel       thrVal;
    private final JLabel       dialVal;
    private final JLabel       statusLabel;
    private final JProgressBar progressBar;

    private volatile int lastDial = -1;

    public DialFrame() {
        setTitle("PNS Virtual Dial");
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setResizable(false);

        Font labelFont = new Font("Monospaced", Font.PLAIN, 14);
        Font valueFont = new Font("Monospaced", Font.BOLD,  14);

        JPanel dataPanel = new JPanel(new GridLayout(6, 2, 10, 8));
        dataPanel.setBorder(BorderFactory.createEmptyBorder(16, 24, 16, 24));

        rotXVal = addRow(dataPanel, "Rot X:", labelFont, valueFont);
        rotYVal = addRow(dataPanel, "Rot Y:", labelFont, valueFont);
        rotZVal = addRow(dataPanel, "Rot Z:", labelFont, valueFont);
        adcVal  = addRow(dataPanel, "ADC:",   labelFont, valueFont);
        thrVal  = addRow(dataPanel, "Thresh:", labelFont, valueFont);
        dialVal = addRow(dataPanel, "Dial:",  labelFont, valueFont);

        rotXVal.setText(String.format("%8.2f  deg/s", -250.0f));
        rotYVal.setText(String.format("%8.2f  deg/s", -250.0f));
        rotZVal.setText(String.format("%8.2f  deg/s", -250.0f));
        adcVal.setText( String.format("%6.3f  V",      3.300f));
        thrVal.setText( String.format("%6.3f  V",      3.300f));
        dialVal.setText("100");

        progressBar = new JProgressBar(0, 100);
        progressBar.setStringPainted(true);
        progressBar.setBorder(BorderFactory.createEmptyBorder(4, 24, 4, 24));
        progressBar.setPreferredSize(new Dimension(0, 28));

        statusLabel = new JLabel("Connecting...", SwingConstants.CENTER);
        statusLabel.setFont(new Font("SansSerif", Font.PLAIN, 12));
        statusLabel.setForeground(Color.GRAY);
        statusLabel.setBorder(BorderFactory.createEmptyBorder(4, 0, 10, 0));

        JPanel content = new JPanel();
        content.setLayout(new BoxLayout(content, BoxLayout.Y_AXIS));
        content.add(dataPanel);
        content.add(progressBar);
        content.add(statusLabel);
        add(content);

        pack();
        setLocationRelativeTo(null);

        new VolumeSync(this).start();
    }

    public int getLastDial() {
        return lastDial;
    }

    private JLabel addRow(JPanel panel, String label, Font lf, Font vf) {
        JLabel l = new JLabel(label);
        l.setFont(lf);
        JLabel v = new JLabel("—");
        v.setFont(vf);
        panel.add(l);
        panel.add(v);
        return v;
    }

    public void updateData(float rotX, float rotY, float rotZ, float adc, float thr, int dial) {
        lastDial = dial;
        SwingUtilities.invokeLater(() -> {
            progressBar.setValue(dial);
            rotXVal.setText(String.format("%8.2f  deg/s", rotX));
            rotYVal.setText(String.format("%8.2f  deg/s", rotY));
            rotZVal.setText(String.format("%8.2f  deg/s", rotZ));
            adcVal.setText( String.format("%6.3f  V",     adc));
            thrVal.setText( String.format("%6.3f  V",     thr));
            dialVal.setText(String.format("%d",           dial));
        });
    }

    public void setStatus(String text, Color color) {
        SwingUtilities.invokeLater(() -> {
            statusLabel.setText(text);
            statusLabel.setForeground(color);
        });
    }
}
