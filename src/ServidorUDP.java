import java.net.DatagramPacket;
import java.net.DatagramSocket;

public class ServidorUDP {
  
  static final int BUFFER = 256;
  public static void main(String[] args) {   
     DatagramSocket socket = null;

    try {
      int port = 1730; //porta
      //Cria o DatagramSocket para aguardar mensagens, neste momento o método fica bloqueando até o recebimente de uma mensagem
      socket = new DatagramSocket(port);
      System.out.println("Ouvindo a porta: " + port);
      
      //Preparando o buffer de recebimento da mensagem
      byte[] buffer = new byte[BUFFER];

      //Prepara o pacote de dados
      DatagramPacket pkg = new DatagramPacket(buffer, buffer.length);
        
      while(true)
      {   
        //Recebimento da mensagem
        socket.receive(pkg);

        System.out.println(new String(pkg.getData()).trim());
      }
    }
    
    catch(Exception e) {
      System.out.println("Erro: " + e.getMessage());
    }
  }
}