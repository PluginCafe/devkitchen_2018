# Creates a document with a cube object then saves it as c4d file.

import os

import c4d
from c4d import documents

def main():
    # Creates a document
    doc = documents.BaseDocument()
    
    # Creates and inserts a cube object
    cube = c4d.BaseObject(c4d.Ocube)
    doc.InsertObject(cube)

    # Generates the document file name
    name = os.path.join(os.path.dirname(__file__), 'cube.c4d')

    # Saves the document
    saved = documents.SaveDocument(doc, name, c4d.SAVEDOCUMENTFLAGS_0, c4d.FORMAT_C4DEXPORT)
    if saved:
        print('Succesfully saved document to \'{}\''.format(name))


if __name__=='__main__':
    main()
