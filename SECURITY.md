# Política de Segurança

## Reportando uma vulnerabilidade

Levamos vulnerabilidades de segurança no G-Portugol a sério. Se você encontrar
um problema de segurança, reporte-o de forma responsável.

**Não abra uma issue pública no GitHub para relatar uma vulnerabilidade.**

Em vez disso, use o recurso de relato privado de vulnerabilidade do GitHub:

1. Abra a aba **Security** do repositório;
2. Selecione **Report a vulnerability**;
3. Envie os detalhes pelo formulário de GitHub Security Advisory.

Inclua o máximo de informações possível para permitir a reprodução e análise:

- descrição da vulnerabilidade;
- passos para reproduzir o problema;
- impacto potencial;
- prova de conceito, se disponível;
- versão do G-Portugol, sistema operacional, arquitetura e modo de execução
  utilizado (interpretador, compilador, assembly ou tradutor C).

Confirmaremos o recebimento do relatório assim que possível e trabalharemos com
você para entender e corrigir o problema.

## Divulgação responsável

Pedimos que siga práticas de divulgação responsável:

- não divulgue publicamente a vulnerabilidade antes que ela seja corrigida;
- conceda aos mantenedores tempo razoável para investigar e preparar uma
  correção;
- coordene conosco a publicação de advisories, CVEs, posts ou outros materiais
  públicos.

Após a correção, reconheceremos sua contribuição publicamente, a menos que você
prefira permanecer anônimo.

## Versões com suporte

Correções de segurança são fornecidas prioritariamente para a versão estável
mais recente.

| Versão | Suporte |
| --- | --- |
| Última versão estável | ✅ |
| Versões estáveis anteriores | ⚠️ Melhor esforço |
| Versões não suportadas | ❌ |

Recomendamos manter instalações do G-Portugol atualizadas.

## Considerações para execução segura

Programas G-Portugol podem ser interpretados ou compilados para binários
nativos. Trate código de origem de terceiros como não confiável: revise-o e
execute-o em ambiente isolado, com permissões restritas, quando não confiar em
sua procedência. Isso também se aplica aos binários e ao código C gerados pelo
compilador.

## Agradecimentos

Agradecemos pesquisadores de segurança e a comunidade de código aberto por
relatos responsáveis que ajudam a tornar o G-Portugol mais seguro.
