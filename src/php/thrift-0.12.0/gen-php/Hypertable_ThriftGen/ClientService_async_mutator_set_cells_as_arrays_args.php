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

class ClientService_async_mutator_set_cells_as_arrays_args
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
            'etype' => TType::LST,
            'elem' => array(
                'type' => TType::LST,
                'etype' => TType::STRING,
                'elem' => array(
                    'type' => TType::STRING,
                    ),
                ),
        ),
    );

    /**
     * @var int
     */
    public $mutator = null;
    /**
     * @var string[][]
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
        return 'ClientService_async_mutator_set_cells_as_arrays_args';
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
                        $_size333 = 0;
                        $_etype336 = 0;
                        $xfer += $input->readListBegin($_etype336, $_size333);
                        for ($_i337 = 0; $_i337 < $_size333; ++$_i337) {
                            $elem338 = null;
                            $elem338 = array();
                            $_size339 = 0;
                            $_etype342 = 0;
                            $xfer += $input->readListBegin($_etype342, $_size339);
                            for ($_i343 = 0; $_i343 < $_size339; ++$_i343) {
                                $elem344 = null;
                                $xfer += $input->readString($elem344);
                                $elem338 []= $elem344;
                            }
                            $xfer += $input->readListEnd();
                            $this->cells []= $elem338;
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
        $xfer += $output->writeStructBegin('ClientService_async_mutator_set_cells_as_arrays_args');
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
            $output->writeListBegin(TType::LST, count($this->cells));
            foreach ($this->cells as $iter345) {
                $output->writeListBegin(TType::STRING, count($iter345));
                foreach ($iter345 as $iter346) {
                    $xfer += $output->writeString($iter346);
                }
                $output->writeListEnd();
            }
            $output->writeListEnd();
            $xfer += $output->writeFieldEnd();
        }
        $xfer += $output->writeFieldStop();
        $xfer += $output->writeStructEnd();
        return $xfer;
    }
}
