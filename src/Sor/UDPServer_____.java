import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketException;
import java.io.IOException;

public class UDPServer_____ {

    private static final int ECHOMAX = 255; // Tamanho máximo da mensagem
    private static final int PORT = 6000;   // Porta de comunicação

    public static void main(String[] args) {
        DatagramSocket socket = null;

        try {
            // Cria o socket UDP na porta especificada
            socket = new DatagramSocket(PORT);
            System.out.println("Esperando Mensagens...");

            byte[] buffer = new byte[ECHOMAX];
            DatagramPacket receivePacket = new DatagramPacket(buffer, buffer.length);

            String message;
            do {
                // Recebe pacote de datagrama
                socket.receive(receivePacket);

                // Extrai a mensagem
                message = new String(receivePacket.getData(), 0, receivePacket.getLength());
                System.out.println(message);

            } while (!message.equals("exit"));

        } catch (SocketException e) {
            System.out.println("ERRO na Criação do Socket!");
        } catch (IOException e) {
            System.out.println("Erro ao receber dados: " + e.getMessage());
        } finally {
            if (socket != null && !socket.isClosed()) {
                socket.close();
            }
        }
    }
}
