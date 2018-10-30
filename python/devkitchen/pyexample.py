import maxon
from c4d import BaseObject

from maxon.datatype import PtrMove, Ptr, Result
from maxon import MAXON_INTERFACE_NONVIRTUAL, MAXON_STATICMETHOD, consts

@MAXON_INTERFACE_NONVIRTUAL(consts.MAXON_REFERENCE_NONE, "devkitchen.python.exampleinterface")
class ExampleInterface(maxon.Data):

    @staticmethod
    @MAXON_STATICMETHOD("devkitchen.python.exampleinterface.CreateObject", annotations=[
                                                                            None, # first param, don't need casting
                                                                            Result[Ptr[BaseObject]] # Result
                                                                            ])
    def CreateObject():
        pass