public class VolumeSync extends Thread {
    //had to make this a separate thread. attatching it to the swing window doesnt update fast enough.
    private final DialFrame frame;

    public VolumeSync(DialFrame frame) {
        this.frame = frame; //bring frame into scope
        setDaemon(true);
    }

    @Override
    public void run() {
        int applied = -1;
        while (true) {
            int target = frame.getLastDial();
            if (target >= 0 && target != applied) {
                try {
                    new ProcessBuilder("osascript", "-e", "set volume output volume " + target)
                        .start()
                        .waitFor();
                    applied = target;
                } catch (Exception e) {
                    System.out.println("Volume set failed: " + e.getMessage());
                }
            } else {
                try { Thread.sleep(30); } catch (InterruptedException ignored) {}
            }
        }
    }
}
