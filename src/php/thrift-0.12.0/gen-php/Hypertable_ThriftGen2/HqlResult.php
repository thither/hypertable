<?php
namespace Hypertable_ThriftGen2;

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

/**
 * Result type of HQL queries
 * 
 * <dl>
 *   <dt>results</dt>
 *   <dd>String results from metadata queries</dd>
 * 
 *   <dt>cells</dt>
 *   <dd>Resulting table cells of for buffered queries</dd>
 * 
 *   <dt>scanner</dt>
 *   <dd>Resulting scanner ID for unbuffered queries</dd>
 * 
 *   <dt>mutator</dt>
 *   <dd>Resulting mutator ID for unflushed modifying queries</dd>
 * </dl>
 */
class HqlResult
{
    static public $isValidate = false;

    static public $_TSPEC = array(
        1 => array(
            'var' => 'results',
            'isRequired' => false,
            'type' => TType::LST,
            'etype' => TType::STRING,
            'elem' => array(
                'type' => TType::STRING,
                ),
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
        3 => array(
            'var' => 'scanner',
            'isRequired' => false,
            'type' => TType::I64,
        ),
        4 => array(
            'var' => 'mutator',
            'isRequired' => false,
            'type' => TType::I64,
        ),
    );

    /**
     * @var string[]
     */
    public $results = null;
    /**
     * @var \Hypertable_ThriftGen\Cell[]
     */
    public $cells = null;
    /**
     * @var int
     */
    public $scanner = null;
    /**
     * @var int
     */
    public $mutator = null;

    public function __construct($vals = null)
    {
        if (is_array($vals)) {
            if (isset($vals['results'])) {
                $this->results = $vals['results'];
            }
            if (isset($vals['cells'])) {
                $this->cells = $vals['cells'];
            }
            if (isset($vals['scanner'])) {
                $this->scanner = $vals['scanner'];
            }
            if (isset($vals['mutator'])) {
                $this->mutator = $vals['mutator'];
            }
        }
    }

    public function getName()
    {
        return 'HqlResult';
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
                    if ($ftype == TType::LST) {
                        $this->results = array();
                        $_size0 = 0;
                        $_etype3 = 0;
                        $xfer += $input->readListBegin($_etype3, $_size0);
                        for ($_i4 = 0; $_i4 < $_size0; ++$_i4) {
                            $elem5 = null;
                            $xfer += $input->readString($elem5);
                            $this->results []= $elem5;
                        }
                        $xfer += $input->readListEnd();
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 2:
                    if ($ftype == TType::LST) {
                        $this->cells = array();
                        $_size6 = 0;
                        $_etype9 = 0;
                        $xfer += $input->readListBegin($_etype9, $_size6);
                        for ($_i10 = 0; $_i10 < $_size6; ++$_i10) {
                            $elem11 = null;
                            $elem11 = new \Hypertable_ThriftGen\Cell();
                            $xfer += $elem11->read($input);
                            $this->cells []= $elem11;
                        }
                        $xfer += $input->readListEnd();
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 3:
                    if ($ftype == TType::I64) {
                        $xfer += $input->readI64($this->scanner);
                    } else {
                        $xfer += $input->skip($ftype);
                    }
                    break;
                case 4:
                    if ($ftype == TType::I64) {
                        $xfer += $input->readI64($this->mutator);
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
        $xfer += $output->writeStructBegin('HqlResult');
        if ($this->results !== null) {
            if (!is_array($this->results)) {
                throw new TProtocolException('Bad type in structure.', TProtocolException::INVALID_DATA);
            }
            $xfer += $output->writeFieldBegin('results', TType::LST, 1);
            $output->writeListBegin(TType::STRING, count($this->results));
            foreach ($this->results as $iter12) {
                $xfer += $output->writeString($iter12);
            }
            $output->writeListEnd();
            $xfer += $output->writeFieldEnd();
        }
        if ($this->cells !== null) {
            if (!is_array($this->cells)) {
                throw new TProtocolException('Bad type in structure.', TProtocolException::INVALID_DATA);
            }
            $xfer += $output->writeFieldBegin('cells', TType::LST, 2);
            $output->writeListBegin(TType::STRUCT, count($this->cells));
            foreach ($this->cells as $iter13) {
                $xfer += $iter13->write($output);
            }
            $output->writeListEnd();
            $xfer += $output->writeFieldEnd();
        }
        if ($this->scanner !== null) {
            $xfer += $output->writeFieldBegin('scanner', TType::I64, 3);
            $xfer += $output->writeI64($this->scanner);
            $xfer += $output->writeFieldEnd();
        }
        if ($this->mutator !== null) {
            $xfer += $output->writeFieldBegin('mutator', TType::I64, 4);
            $xfer += $output->writeI64($this->mutator);
            $xfer += $output->writeFieldEnd();
        }
        $xfer += $output->writeFieldStop();
        $xfer += $output->writeStructEnd();
        return $xfer;
    }
}
