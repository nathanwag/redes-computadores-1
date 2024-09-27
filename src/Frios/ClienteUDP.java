package Frios;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;

public class ClienteUDP {
    public static void main(String[] args) {
        
        // if(args.length != 3) {
        //     System.out.println("Uso correto: <Nome da maquina> <Porta> <Mensagem>");
        //     System.exit(0);
        // }

        String ip = Input.inputString("Digite o IP do servidor: ");
        int port = Input.inputInt("Digite a porta: ");
        System.out.println("Digite as mensagens para serem enviadas: ");
        
        try {
            while (true) {
                String inputMsg = Input.inputString("");
                //Primeiro argumento é o nome do host destino
                InetAddress addr = InetAddress.getByName(ip);
                byte[] msg = inputMsg.getBytes();
                //Monta o pacote a ser enviado
                DatagramPacket pkg = new DatagramPacket(msg,msg.length, addr, port);
                // Cria o DatagramSocket que será responsável por enviar a mensagem
                DatagramSocket ds = new DatagramSocket();
                //Envia a mensagem
                ds.send(pkg);
                System.out.println("Mensagem enviada para: " + addr.getHostAddress() +
                ":" + port);
            }
        }
        
        catch(Exception e) {
            System.out.println("Erro: " + e.getMessage());
        }
    }
}
