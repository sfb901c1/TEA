This is the prototype of an anonymous communication system.
Most of the functioning of this software cannot be understood without the explanations given in the corresponding paper (not yet published).

### Abstract:
TEA is a prototype instance of a provably secure anonymous communication system. 
In this system, each node can generate a number of pseudonyms unlinkable to the node that created them.
 Nodes can send message from and to pseudonyms, allowing both senders and receivers of messages to remain anonymous.

To achieve the provably high security, the anonymous communication system requires each node to be equipped with a Trusted Execution Environment (TEE). 
 Assuming that the TEE does what it should, the theorems proven for the anonymous communication system equally apply to TEA: 
 The system guarantees strong anonymity in spite of the presence of an adversary that observes all network traffic and can adaptively but with a small delay corrupt a constant fraction of the participating nodes. 
 This means the adversary could completely overtake the nodes' operating systems, block incoming or outgoing messages, and even tamper with the binary code of the executable. 
 The only part the adversary has no access to is the code and memory of the sealed TEE enclave.

### General architecture description:
* The system consists of two parts: A login server used for bootstrapping the peers, and the peers.
* Each part consists of an untrusted and trusted (inside enclave) part.
* Although the login server is not supposed to be there in the final system, it has an enclave part to allow for secure connection with the peers for the time being.
* In many places, you will notice that a former version of this software ran on Intel SGX. This influenced the architectural design of this software.

### Howto:
  * use cmake to build (or "docker build .")
  * run login_server (the login server)
  * start 81 clients
  * use the client\_interface binary for user input to the clients (generate_pseudonym, etc.)

### Known Limitations:
  * currently hardcoded for n = 81 nodes (overlay network with 8 (quorum) nodes, i.e., dimension 3)
  
### Required packages for development (package names for debian-based systems):
  * libboost-test-dev
  * libcurl4-openssl-dev
  * libcurlpp-dev
  * libjsoncpp-dev
  * libzmq3-dev
  * for the GUI: qtbase5-dev
  * for the visualization: python3, python3-tk, python3-waitress (+ via pip: falcon, ujson, numpy, matplotlib)
