# Proyecto2-Empotrados



Este error es muy intimidante (¡363 errores!), pero en realidad es uno solo repetido muchas veces. Ocurre casi siempre la primera vez que intentas compilar un diseño con el procesador HPS (ARM) en Quartus.

El problema es que te falta correr el script de asignación de pines del HPS.

El Diagnóstico: El mensaje clave está al final de tu log: Info (169066): Type bi-directional pin memory_mem_dqs uses the 2.5 V I/O standard

La memoria RAM del procesador ARM (HPS) es DDR3, la cual funciona a 1.5V (o 1.35V). Quartus, por defecto, configura todos los pines no asignados a 2.5V. Como hay un desajuste de voltaje (Hardware dice 1.5V, Configuración dice 2.5V), Quartus desconecta los circuitos de calibración (Termination Control), y por eso te salen los errores de PARALLELTERMINATIONCONTROL.

Solución: Ejecutar el Script TCL "Mágico"
Cuando generaste tu sistema en Platform Designer (Qsys), la herramienta creó automáticamente un script .tcl diseñado específicamente para arreglar esto (asignar los voltajes, ubicaciones y estándares correctos a los pines de la memoria DDR).

Sigue estos pasos:

En Quartus Prime, ve al menú superior: Tools -> Tcl Scripts...

En la lista que aparece, busca un archivo que se llame:

hps_sdram_p0_pin_assignments.tcl

(A veces se llama tu_sistema_pin_assignments.tcl).

Selecciónalo y haz clic en Run.

Deberías ver en la consola de mensajes de Quartus que empieza a asignar pines (Assigning pin...).

Cuando termine, cierra esa ventana y vuelve a compilar todo (Start Compilation).