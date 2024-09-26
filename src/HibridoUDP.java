import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;

public class HibridoUDP {
    static final int BUFFER = 256;
    static byte[] buffer = new byte[BUFFER];

    private static void listener(DatagramSocket socket, String ip, int port) {
        System.out.println("Ouvindo a porta: " + port);
        DatagramPacket pkg = new DatagramPacket(buffer, buffer.length);
        try {
            while(!socket.isClosed()) {   
                socket.receive(pkg);
                System.out.println(new String(pkg.getData()).trim());
                socket.close();
              }
        }
        catch(Exception e) {
            System.out.println("Erro: " + e.getMessage());
        }
    }

    public static void main(String[] args) {
        String ip = "172.20.10.7";
        int port = 1730;
        try {
            DatagramSocket socket = new DatagramSocket(port);
            String inputMsg = Input.inputString("");
            InetAddress addr = InetAddress.getByName(ip);
            byte[] msg = inputMsg.getBytes();
            DatagramPacket pkg = new DatagramPacket(msg,msg.length, addr, port);
            DatagramSocket ds = new DatagramSocket();
            ds.send(pkg);
            System.out.println("Mensagem enviada para: " + addr.getHostAddress() +
            ":" + port);
            listener(ds, inputMsg, port);
        }
        catch (Exception e) {
            System.out.println("Erro: " + e.getMessage());
        }
    }
}