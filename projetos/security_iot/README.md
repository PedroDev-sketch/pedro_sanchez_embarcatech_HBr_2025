# Relato e Análise Final
## 1. Técnicas Implementadas
Autenticação MQTT: Uso de usuário/senha (aluno/senha123) no broker para controle de acesso.

Criptografia Leve: Cifra XOR (para fins didáticos) e TLS/SSL (recomendado em produção).

Proteção contra Ataques:

    Sniffing: Evitado com criptografia (TLS ideal, XOR como exemplo).

    Replay: Timestamp (ts) incluso nos dados para invalidar mensagens antigas.

## 2. Escalabilidade
Broker Centralizado: Um broker MQTT (ex: Mosquitto) pode gerenciar múltiplas BitDogLabs.

Tópicos Hierárquicos: Ex: escola/sala1/temperatura, escola/sala2/umidade, facilitando organização.

Autenticação em Lote: Usar certificados TLS ou credenciais únicas por dispositivo.

## 3. Aplicação em Rede Escolar
Cenário: 10 BitDogLabs monitorando salas.

Broker Único: Configurado com ACL (Access Control Lists) para limitar tópicos por dispositivo.

Criptografia TLS: Obligatória para comunicação segura entre dispositivos e broker.

Monitoramento: Ferramentas como Node-RED para dashboard em tempo real.

## 4. Desafios e Melhorias
Desafio: XOR é inseguro para produção → substituir por AES-128/256.

Melhoria: Implementar MQTT over WebSocket para acesso via navegador.

## Link do Vídeo de Demonstração
https://youtube.com/shorts/luxRLE_m9JU?si=8MBsZ9qioEjoKNuM