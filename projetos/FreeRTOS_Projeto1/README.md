# Relatório Projeto FreeRTOS

## O que acontece se todas as tarefas tiverem a mesma prioridade?

    Quando todas as tarefas em um sistema FreeRTOS têm a mesma prioridade, o agendador do FreeRTOS utiliza um 
algoritmo de round-robin preemptivo. Isso significa que cada tarefa de mesma prioridade recebe uma fatia de tempo da 
CPU por vez. Após o término do seu time slice, ou se a tarefa ceder voluntariamente a CPU (por exemplo, chamando 
vTaskDelay() ou xQueueReceive()), o agendador alterna para a próxima tarefa de mesma prioridade na fila. Esse processo se repete ciclicamente, garantindo que todas as tarefas de igual prioridade tenham a oportunidade de executar.


## Qual tarefa consome mais tempo da CPU?

    A tarefa que mais consome tempo da CPU é a vButtonControl, pois enquanto todas as tarefas cedem a CPU com vTaskDelay, a 
vButtonControl realiza verificações de polling de forma mais frequente e incondicional (a cada 100ms) em comparação com o 
"trabalho" intermitente das tarefas de LED e buzzer. Portanto, ela passará uma proporção maior do tempo total de execução da CPU 
realizando suas operações de leitura de GPIO e lógica de controle, mesmo que o volume de código executado em cada ciclo seja pequeno.


## Quais seriam os riscos de usar polling sem prioridades?

Usar polling sem prioridades no FreeRTOS apresenta riscos significativos, pois ele consome muitos ciclos da CPU ao verificar continuamente o estado de algo, mesmo quando não há mudanças. Isso leva a:

    * Alto Consumo de CPU: A tarefa de polling pode monopolizar a CPU, desperdiçando recursos.

    * Latência Inconsistente: Outras tarefas importantes podem sofrer atrasos, pois precisam esperar o polling terminar.

    * Desperdício de Energia: A CPU fica sempre ativa, impedindo modos de economia de energia.

    * Complexidade de Manutenção: Escalar e gerenciar o sistema fica mais difícil à medida que mais tarefas são adicionadas.

Em vez de polling, é muito mais eficiente usar interrupções ou primitivas de sincronização do FreeRTOS para ativar tarefas apenas quando um evento real ocorre.