# Projeto Condutividade
Repositório do projeto de pesquisa sobre um sensor de condutividade acessível. (2019-2020)

# Composição do Repositório
Este repositório é composto por vários projetos que, ao entrarem em execução, interagem entre si.

## Código do Arduino Uno
Código que opera a unidade Arduino Uno responsável por fazer ligação direta com o sensor e tratar das rotinas de operação do mesmo, repassando os dados para o módulo ESP-01/ESP8266. (Back-End)

## Código do Arduino ESP-01/ESP8266
Código que configura a ponte entre o Arduino Uno e a plataforma ThingSpeak para que os dados coletados do sensor possam ser enviados para a nuvem de acordo com as requisições apontadas pela plataforma. (Back-End)

## Código do Aplicativo Android
Arquivo de projeto de aplicativo Android da Plataforma Mit App Inventor 2 usado para obter os dados de leituras dos sensores através da plataforma ThingSpeak. (Front-End)

# Feito com empenho e dedicação por
* Robson Prucoli Posse
* Julio Cesar Goldner Vendramini
* Sabrina Gobbi Scaldaferro
* Victor Barcelos Lacerda
