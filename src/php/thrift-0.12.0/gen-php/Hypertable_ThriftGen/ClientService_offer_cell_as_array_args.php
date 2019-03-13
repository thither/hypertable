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

class ClientService_offer_cell_as_array_args
{
    static public $isValidate = false;

    static public $_TSPEC = array(
        1 => array(
            'var' => 'ns',
            'isRequired' => false,
            'type' => TType::I64,
        ),
        2 => array(
            'var' => 'table_name',
            'isRequired' => false,
            'type' => TType::STRING,
        ),
        3 => array(
            'var' => 'mutate_spec',
            'isRequired' => false,
            'type' => TType::STRUCT,
            'class' => '\Hypertable_ThriftGen\MutateSpec',
        ),
        4 => array(
            'var' => 'cell',
            'isRequired' => false,
            'type' => TType::LST,
            'etype' => TType::STRING,
            'elem' => array(
                'type' => TType::STRING,
                ),
        ),
    );

    /**
     * @var int
     */
    public $ns = null;
    /**
     * @var string
     */
    public $table_name = null;
    /**
     * @var \Hypertable_ThriftGen\MutateSpec
     */
    public $mutate_spec = null;
    /**
     * @var string[]
     */
    public $cell = null;

    public function __construct($vals = null)
    {
        if (is_array($vals)) {
            if (isset($vals['ns'])) {
                $this->ns = $vals['ns'];
            }
            if (isset($vals['table_name'])) {
                $this->table_name = $vals['table_name'];
            }
            if (isset($vals['mutate_spec'])) {
                $this->mutate_spec = $vals['mutate_spec'];
            }
            if (isset($vals['cell'])) {
                $this->cell = $vals['cell'];
            }
        }
    }

    public function getName()
    {
        return 'ClientService_offer_cell_as_array_args';
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
                        $xfer += $input->readI64($this->ns);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 2:
                    if ($ftype == TType::STRING) {
                        $xfer += $input->readString($this->table_name);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 3:
                    if ($ftype == TType::STRUCT) {
                        $this->mutate_spec = new \Hypertable_ThriftGen\MutateSpec();
                        $xfer += $this->mutate_spec->read($input);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 4:
                    if ($ftype == TType::LST) {
                        $this->cell = array();
                        $_size242 = 0;
                        $_etype245 = 0;
                        $xfer += $input->readListBegin($_etype245, $_size242);
                        for ($_i246 = 0; $_i246 < $_size242; ++$_i246) {
                            $elem247 = null;
                            $xfer += $input->readString($elem247);
                            $this->cell []= $elem247;
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
        $xfer += $output->writeStructBegin('ClientService_offer_cell_as_array_args');
        if ($this->ns !== null) {
            $xfer += $output->writeFieldBegin('ns', TType::I64, 1);
            $xfer += $output->writeI64($this->ns);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->table_name !== null) {
            $xfer += $output->writeFieldBegin('table_name', TType::STRING, 2);
            $xfer += $output->writeString($this->table_name);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->mutate_spec !== null) {
            if (!is_object($this->mutate_spec)) {
                throw new TProtocolException('Bad type in structure.', TProtocolException::INVALID_DATA);
            }
            $xfer += $output->writeFieldBegin('mutate_spec', TType::STRUCT, 3);
            $xfer += $this->mutate_spec->write($output);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->cell !== null) {
            if (!is_array($this->cell)) {
                throw new TProtocolException('Bad type in structure.', TProtocolException::INVALID_DATA);
            }
            $xfer += $output->writeFieldBegin('cell', TType::LST, 4);
            $output->writeListBegin(TType::STRING, count($this->cell));
            foreach ($this->cell as $iter248) {
                $xfer += $output->writeString($iter248);
            }
            $output->writeListEnd();
            $xfer += $output->writeFieldEnd();
        }
        $xfer += $output->writeFieldStop();
        $xfer += $output->writeStructEnd();
        return $xfer;
    }
}
