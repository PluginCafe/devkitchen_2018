import c4d
import maxon
import devkitchen

# Main function
def main():
    objType = maxon.config.yourCustomConfiguration

    obj = devkitchen.ExampleInterface.CreateObject(objType)
    # obj = devkitchen.ExampleInterface.CreateObject(maxon.OBJECTTYPE.CUBE)

    doc.InsertObject(obj)
    c4d.EventAdd()


# Execute main()
if __name__=='__main__':
    main()