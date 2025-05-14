# Atividade 13-05
Este é o repositório que armazena a tarefa solicitada no dia 29/04. Todos os arquivos necessários à execução já foram criados, de modo que basta seguir as instruções abaixo para executá-lo em seu dispositivo.

## Como Usar
1. Para acessar a atividade armazenada, clone este repositório para seu dispositivo executando o seguinte comando no terminal de um ambiente adequado, como o VS Code, após criar um repositório local: git clone https://github.com/nivaldojunior037/Atividade-13-05-EmbarcaTech

2. Após isso, importe como um projeto a pasta que contém os arquivos clonados em um ambiente como o VS Code, susbtitua, nas linhas 15 e 16 do código os termos SUA_REDE e SUA_SENHA pelo nome e senha davrede wi-fi que deseja utilizar e, por fim, compile o código existente para que os demais arquivos necessários sejam criados em seu dispositivo

3. Ao fim da compilação, será gerado um arquivo .uf2 na pasta build do seu repositório. Esse arquivo deve ser copiado para a BitDogLab em modo BOOTSEL para que ele seja corretamente executado.

### Como Executar o Código
1. Quando o arquivo .uf2 é copiado para a placa, ela tentará se conectar à rede Wi-fi informada de acordo com as instruções anteriores. Realize o monitoramento serial da placa em um ambiente adequado, como o VS Code com a taxa de transmissão de 115200 e verifique o IP fornecido, caso a conexão seja bem sucedida. Além disso, o LED verde central da placa também se acenderá quando a conexão for realizada. 

2. Insira o código IP obtido em um navegador de um celular, tablet ou computador e monitore os dados informados. A temperatura é simulada com o sensor de temperatura da BitDogLab, de modo que não pode ser alterada diretamente. A saturação é controlada pelos potenciômetros do Joystick e a frequência cardíaca é controlada pelos botões A e B. 

3. Atenção: A Atualização da página ainda não é automática, de modo que é necessário atualizá-la para poder continuar a monitorar as alterações nos sinais simulados. 

4. Há também na placa indicadores de sinais vitais, a exemplo do LED RGB e do buzzer, que possuem respostas específicas para quando a frequência cardíaca ou a saturação atingem níveis preocupantes ou críticos. Esses intervalos são os mesmos considerados paara atualização das mensagens e cor de fundo do webserver. 

#### Link do vídeo
Segue o link do Drive com o vídeo onde é demonstrada a utilização do código: https://drive.google.com/drive/folders/1GtNroR81hMSkEAoeBoWfBaoPZjBi2sZl?usp=sharing