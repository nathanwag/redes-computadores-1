# Use uma imagem base com Java (OpenJDK 17)
FROM openjdk:17

# Cria um diretório dentro do container para armazenar o código
WORKDIR /app

# Copia o código Java do seu host para o container
COPY . /app

# Compila o código Java
RUN javac UDPServerClient.java

# Comando para rodar o programa Java
CMD ["java", "UDPServerClient"]