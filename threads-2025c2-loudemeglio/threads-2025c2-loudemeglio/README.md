# Threads

Lourdes De Meglio 
# Justificaciones

* Para la comunicación entre los threads Receiver y del GameLoop utilizo una cola non-blocking, ya que el loop del juego no puede
bloquearse esperando mensajes, en caso que no haya mensajes debe seguir iterando, y unbounded ya que no hay un limite de mensajes que podemos 
recibir de los clientes, además que no querríamos que estos se pierdan. 
* Para la comunicación entre el gameloop y el hilo emisor Sender de cada cliente utilizo una cola unbounded y blocking, ya que el emisor puede 
quedarse bloqueado esperando mensajes sin afectar la simulación del juego, además al hacerla unbounded no perderemos eventos que ovurren en él.  

# Aclaraciones

Clases como Socket, Thread, Queue, entre otras, fueron provistas por la cátedra Veiga en la materia Taller de programación:

Autor: eldipa

Licencia: GPL v2

Referencias: 
* https://github.com/eldipa/hands-on-sockets-in-cpp
* https://github.com/eldipa/hands-on-threads

