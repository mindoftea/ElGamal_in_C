### ElGamal Cryptosystem
## With Long Integer Computation Library

Emory Hufbauer [originally authored 04/21/16]

NOT FOR PRODUCTION USE

The included makefile is designed for a Linux system.
It creates three executables:
	- ./keyGenerator key-size private-key-file public-key-file
		Generate a public-private key pair of key-size bits and place them in private-key-file and public-key-file, respectively.
	- ./encryptor public-key-file ciphertext-output-file < plaintext-input-file
		Encrypt data from stdin using the key stored in public-key-file and save it to ciphertext-output-file.
	- ./decryptor private-key-file ciphertext-input-file > plaintext-output-file
		Decrypt data from ciphertext-input-file using the key stored in private-key-file and write it to stdout.