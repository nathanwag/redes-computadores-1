import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.io.IOException;
import java.util.Scanner;

public class UDPClient_____ {

    //private static final int ECHOMAX = 255; // Tamanho máximo da mensagem
    private static final int PORT = 6000;   // Porta de destino

    public static void main(String[] args) {
        // Verifica se o IP do servidor foi fornecido como argumento
        if (args.length != 1) {
            System.out.println("Usage: java UDPClient <Server IP>");
            return;
        }

        String servIP = args[0];
        DatagramSocket socket = null;
        Scanner scanner = new Scanner(System.in);

        try {
            // Cria o socket UDP
            socket = new DatagramSocket();

            InetAddress serverAddress = InetAddress.getByName(servIP);

            String message;
            do {
                // Lê a entrada do usuário
                System.out.print("Digite a mensagem: ");
                message = scanner.nextLine();

                // Envia o pacote para o servidor
                byte[] buffer = message.getBytes();
                DatagramPacket sendPacket = new DatagramPacket(buffer, buffer.length, serverAddress, PORT);
                socket.send(sendPacket);

            } while (!message.equals("exit"));

        } catch (SocketException e) {
            System.out.println("Socket falhou: " + e.getMessage());
        } catch (IOException e) {
            System.out.println("Erro ao enviar dados: " + e.getMessage());
        } finally {
            if (socket != null && !socket.isClosed()) {
                socket.close();
            }
            scanner.close();
        }
    }
}
