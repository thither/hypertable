<?php
namespace Hypertable_ThriftGen;

/**
 * Autogenerated by Thrift Compiler (0.12.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
use Thrift\Base\TBase;
use Thrift\Type\TType;
use Thrift\Type\TMessageType;
use Thrift\Exception\TException;
use Thrift\Exception\TProtocolException;
use Thrift\Protocol\TProtocol;
use Thrift\Protocol\TBinaryProtocolAccelerated;
use Thrift\Exception\TApplicationException;

class ClientService_set_cells_async_args
{
    static public $isValidate = false;

    static public $_TSPEC = array(
        1 => array(
            'var' => 'mutator',
            'isRequired' => false,
            'type' => TType::I64,
        ),
        2 => array(
            'var' => 'cells',
            'isRequired' => false,
            'type' => TType::LST,
            'etype' => TType::STRUCT,
            'elem' => array(
                'type' => TType::STRUCT,
                'class' => '\Hypertable_ThriftGen\Cell',
                ),
        ),
    );

    /**
     * @var int
     */
    public $mutator = null;
    /**
     * @var \Hypertable_ThriftGen\Cell[]
     */
    public $cells = null;

    public function __construct($vals = null)
    {
        if (is_array($vals)) {
            if (isset($vals['mutator'])) {
                $this->mutator = $vals['mutator'];
            }
            if (isset($vals['cells'])) {
                $this->cells = $vals['cells'];
            }
        }
    }

    public function getName()
    {
        return 'ClientService_set_cells_async_args';
    }


    public function read($input)
    {
        $xfer = 0;
        $fname = null;
        $ftype = 0;
        $fid = 0;
        $xfer += $input->readStructBegin($fname);
        while (true) {
            $xfer += $input->readFieldBegin($fname, $ftype, $fid);
            if ($ftype == TType::STOP) {
                break;
            }
            switch ($fid) {
                case 1:
                    if ($ftype == TType::I64) {
                        $xfer += $input->readI64($this->mutator);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 2:
                    if ($ftype == TType::LST) {
                        $this->cells = array();
                        $_size326 = 0;
                        $_etype329 = 0;
                        $xfer += $input->readListBegin($_etype329, $_size326);
                        for ($_i330 = 0; $_i330 < $_size326; ++$_i330) {
                            $elem331 = null;
                            $elem331 = new \Hypertable_ThriftGen\Cell();
                            $xfer += $elem331->read($input);
                            $this->cells []= $elem331;
                        }
                        $xfer += $input->readListEnd();
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                default:
                    $xfer += $input->skip($ftype);
                    break;
            }
            $xfer += $input->readFieldEnd();
        }
        $xfer += $input->readStructEnd();
        return $xfer;
    }

    public function write($output)
    {
        $xfer = 0;
        $xfer += $output->writeStructBegin('ClientService_set_cells_async_args');
        if ($this->mutator !== null) {
            $xfer += $output->writeFieldBegin('mutator', TType::I64, 1);
            $xfer += $output->writeI64($this->mutator);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->cells !== null) {
            if (!is_array($this->cells)) {
                throw new TProtocolException('Bad type in structure.', TProtocolException::INVALID_DATA);
            }
            $xfer += $output->writeFieldBegin('cells', TType::LST, 2);
            $output->writeListBegin(TType::STRUCT, count($this->cells));
            foreach ($this->cells as $iter332) {
                $xfer += $iter332->write($output);
            }
            $output->writeListEnd();
            $xfer += $output->writeFieldEnd();
        }
        $xfer += $output->writeFieldStop();
        $xfer += $output->writeStructEnd();
        return $xfer;
    }
}
