# Autoevaluación – Samuel Molina Garcés

**Proyecto 2 – System Calls en xv6**
**Nota que considero merecer: 5.0 / 5.0**

## ¿Por qué considero que merezco esta nota?

1. **Todo lo que se pidió funciona.** `trace` muestra el PID, el nombre de la syscall, el valor de retorno y los registros, `sysinfo` muestra la memoria libre, las páginas usadas y disponibles, y los procesos RUNNABLE. Ambas compilan sin errores y las probé en el entorno de ejecución de QEMU.

2. **Las probé a fondo, no solo el caso base del PDF instructivo.** Verifiqué que los resultados fueran correctos y comprobables, por ejemplo, las páginas usadas y las disponibles siempre suman lo mismo y vuelven a su valor inicial cuando termina un proceso, lo que confirma que no se pierde memoria. También probé los casos de error (nombre de syscall inválido, comando que no existe, faltan argumentos), comprobando así que el programa avisa con un mensaje claro y termina de forma controlada con un código distinto de cero.

3. **No dañé el funcionamiento normal de xv6.** Después de los cambios de este nuevo proyecto, los comandos normales como `ls`, `cat`, `echo`, las redirecciones y las tuberías siguen funcionando igual.

4. **Documenté y declaré todo.** Participé en todas las partes del proyecto (desarrollo, pruebas y documentación). Implementé un buen uso de la IA como apoyo, el código lo revisé y lo probé con ayuda de mi compañero, y puedo explicar el funcionamiento de las nuevas syscalls.