import java.net.*;
import java.io.*;
import java.util.Scanner;

public class UDPServerClient {
    public static void main(String[] args) {
        int portaServidor = 8888;

        // Thread para rodar o servidor UDP
        new Thread(() -> {
            try (DatagramSocket socketServidor = new DatagramSocket(portaServidor)) {
                byte[] receiveData = new byte[1024];

                System.out.println("Servidor UDP pronto, escutando na porta " + portaServidor);

                while (true) {
                    DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
                    socketServidor.receive(receivePacket); // Recebendo dados
                    String mensagemRecebida = new String(receivePacket.getData(), 0, receivePacket.getLength());
                    System.out.println("\n" + receivePacket.getAddress() + ": " + mensagemRecebida);

                    // Enviar resposta ao cliente (opcional)
                    InetAddress enderecoCliente = receivePacket.getAddress();
                    int portaCliente = receivePacket.getPort();
                    String resposta = "certinho";
                    byte[] sendData = resposta.getBytes();
                    DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, enderecoCliente, portaCliente);
                    socketServidor.send(sendPacket); // Enviando resposta
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }).start();

        // Thread para rodar o cliente UDP
        new Thread(() -> {
            try (DatagramSocket socketCliente = new DatagramSocket()) {
                Scanner scanner = new Scanner(System.in);

                while (true) {
                    //System.out.print("Digite uma mensagem para enviar ao servidor: ");
                    String mensagem = scanner.nextLine();
                    byte[] sendData = mensagem.getBytes();

                    InetAddress enderecoServidor = InetAddress.getByName(Input.inputString("Digite o IP do servidor: "));
                    DatagramPacket sendPacket = new DatagramPacket(sendData, sendData.length, enderecoServidor, portaServidor);
                    socketCliente.send(sendPacket); // Enviando dados ao servidor

                    // Receber resposta do servidor (opcional)
                    byte[] receiveData = new byte[1024];
                    DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
                    socketCliente.receive(receivePacket);
                    String respostaServidor = new String(receivePacket.getData(), 0, receivePacket.getLength());
                    System.out.print("  " + respostaServidor);
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }).start();
    }
}
